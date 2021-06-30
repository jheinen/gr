
#include "gks.h"
#include "gkscore.h"
#include "gksquartz.h"
#include "connection_library.h"

#import "GKSTerm.h"
#import "GKSView.h"

#import "log.h"

static void send_nb_message(struct message* message, void *reply, size_t reply_len)
{
    struct message* response_message = get_response_message(message, reply_len, reply, USE_MEMCOPY);
    message_send(response_message);
}
static void nb_handle_create_window(GKSTerm *gksterm, char *data, struct message* message)
{
  (void)data;

  __block int result = 0;
  dispatch_sync(dispatch_get_main_queue(), ^{
    @synchronized(gksterm)
    {
      result = [gksterm GKSQuartzCreateWindow];
    }
  });
  char reply[1 + sizeof(int)];
  DATALENGTH reply_len = sizeof(reply);
  reply[0] = GKSTERM_FUNCTION_CREATE_WINDOW;
  *(int *)(reply + 1) = result;
  send_nb_message(message, reply, reply_len);

  /* Show the app icon in dock */
  ProcessSerialNumber psn = {0, kCurrentProcess};
  TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}

static void nb_handle_draw(GKSTerm *gksterm, char *data, struct message* message)
{
    // Send acknowledgement before actually drawing to avoid timeout
    char reply[1];
    reply[0] = GKSTERM_FUNCTION_DRAW;
    DATALENGTH reply_len = 1;
    send_nb_message(message, reply, reply_len);

     int window = *(int *)data;
     size_t displaylist_len = *(size_t *)(data + sizeof(int));
     void *displaylist = malloc(displaylist_len);
     memcpy(displaylist, (void *)(data + sizeof(int) + sizeof(size_t)), displaylist_len);
     dispatch_async(dispatch_get_main_queue(), ^{
       NSData *displaylist_objc = [NSData dataWithBytesNoCopy:displaylist length:displaylist_len freeWhenDone:NO];
       [gksterm GKSQuartzDraw:window displayList:displaylist_objc];
       free(displaylist);
     });
}

static void nb_handle_is_alive(GKSTerm *gksterm, char *data, struct message* message)
{
  int window = *(int *)data;
  bool result = NO;
  @synchronized(gksterm)
  {
    result = [gksterm GKSQuartzIsAlive:window];
  }
  char reply[2];
  reply[0] = GKSTERM_FUNCTION_IS_ALIVE;
  reply[1] = result ? 1 : 0;
  DATALENGTH reply_len = 2;
  send_nb_message(message, reply, reply_len);
}
static void nb_handle_close_window(GKSTerm *gksterm, char *data, struct message* message)
{
  int window = *(int *)data;
  dispatch_sync(dispatch_get_main_queue(), ^{
    @synchronized(gksterm)
    {
      [gksterm GKSQuartzCloseWindow:window];
    }
  });
  char reply[1];
  reply[0] = GKSTERM_FUNCTION_CLOSE_WINDOW;
  DATALENGTH reply_len = 1;
    send_nb_message(message, reply, reply_len);
}

static void nb_handle_is_running(GKSTerm *gksterm, char *data, struct message* message)
{
  (void)data;
  char reply[1];
  reply[0] = GKSTERM_FUNCTION_IS_RUNNING;
  DATALENGTH reply_len = 1;
  send_nb_message(message, reply, reply_len);
}

static void nb_handle_unknown(char *data, struct message* message)
{
  (void)data;
  char reply[1];
  reply[0] = GKSTERM_FUNCTION_UNKNOWN;
  DATALENGTH reply_len = 1;
  NSLog(@"message with unknown function code");
  send_nb_message(message, reply, reply_len);
}

GKSTerm* gksterm;
void nb_handle_message(struct message* message){
    char request_type = ((char*)(message->data))[0];
    char* data = ((char*)message->data)+1;
    
    switch (request_type)
    {
    case GKSTERM_FUNCTION_CREATE_WINDOW:
        //printf("Request: GKSTERM_FUNCTION_CREATE_WINDOW\n");
        nb_handle_create_window(gksterm, data, message);
        break;
    case GKSTERM_FUNCTION_DRAW:
        //printf("Request: GKSTERM_FUNCTION_DRAW\n");
        nb_handle_draw(gksterm, data, message);
        break;
    case GKSTERM_FUNCTION_IS_ALIVE:
        //printf("Request: GKSTERM_FUNCTION_IS_ALIVE\n");
        nb_handle_is_alive(gksterm, data, message);
        break;
    case GKSTERM_FUNCTION_CLOSE_WINDOW:
        //printf("Request: GKSTERM_FUNCTION_CLOSE_WINDOW\n");
        nb_handle_close_window(gksterm, data, message);
        break;
    case GKSTERM_FUNCTION_IS_RUNNING:
        //printf("Request: GKSTERM_FUNCTION_IS_RUNNING\n");
        nb_handle_is_running(gksterm, data, message);
        break;
    default:
        nb_handle_unknown(data, message);
        break;
    }
}

struct context_object* nb_context = NULL; /*nb context*/

void init_nb_context(struct context_object* nb_context, int port, char* accepted_clients, time_t diff, time_t limit){
        nb_context = init_context(port, NULL, diff, limit);
}

static bool initialized = NO;

@implementation GKSTerm

- (void)awakeFromNib
{
  int win;

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keepOnDisplay:)
                                               name:@"GKSViewKeepOnDisplayNotification"
                                             object:nil];

  //FILE *fptr;
  //fptr = fopen("/Users/peters/Desktop/grnb/gr/lib/gks/quartz/log_output.txt","w");
  //log_add_fp(fptr, 0);
  if (!initialized)
    {
    int port = 7022;
    time_t diff = 3;
    time_t limit = 10;
    init_context(port, nb_handle_message, diff, limit);


    gksterm = self;

    num_windows = 0;
    curr_win_id = 0;
    for (win = 0; win < MAX_WINDOWS; win++) window[win] = nil;
    }
}

- (int)GKSQuartzCreateWindow
{
  int win = [self getNextWindowID];

  if (win < MAX_WINDOWS)
    {
      curr_win_id = win;
      num_windows++;
      NSRect screenFrame = [[[NSScreen screens] objectAtIndex:0] frame];
      window[win] =
          [[NSWindow alloc] initWithContentRect:NSMakeRect(NSMinX(screenFrame), NSMaxY(screenFrame) - 500, 500, 500)
                                      styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask |
                                                NSResizableWindowMask
                                        backing:NSBackingStoreBuffered
                                          defer:NO];
      [window[win] setBackgroundColor:[NSColor colorWithCalibratedWhite:1 alpha:1]];
      view[win] = [[GKSView alloc] initWithFrame:NSMakeRect(0, 0, 500, 500)];
      [window[win] setContentView:view[win]];
      [window[win] makeFirstResponder:view[win]];
      [window[win] makeKeyAndOrderFront:nil];
      [window[win] setTitle:@"GKSTerm"];
      [window[win] display];

      [view[win] setWinID:win];

      cascadingPoint = [window[win] cascadeTopLeftFromPoint:cascadingPoint];

      close_window[win] = YES;
      [[NSNotificationCenter defaultCenter] addObserver:self
                                               selector:@selector(windowWillClose:)
                                                   name:NSWindowWillCloseNotification
                                                 object:window[win]];
      return win;
    }
  else
    return -1;
}

- (void)windowWillClose:(NSNotification *)notification
{
  int win;

  NSWindow *nswin = [notification object];
  for (win = 0; win < MAX_WINDOWS; win++)
    {
      if (window[win] != nil && close_window[win] && window[win] == nswin)
        {
          window[win] = nil;
        }
    }
}

- (int)GKSQuartzIsAlive:(int)win
{
  return window[win] != nil ? 1 : 0;
}

- (void)GKSQuartzDraw:(int)win displayList:(id)displayList
{
  [view[win] setDisplayList:displayList];
}

- (void)GKSQuartzCloseWindow:(int)win
{
  if (close_window[win])
    {
      if (view[win] != nil)
        {
          [view[win] close];
        }
      if (window[win] != nil)
        {
          [window[win] close];
        }
    }
  view[win] = nil;
  window[win] = nil;

  curr_win_id = win;
}

- (IBAction)cascadeWindows:(id)sender
{
  int i;
  NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
  cascadingPoint = NSMakePoint(NSMinX(screenFrame), NSMaxY(screenFrame));

  for (i = 0; i < num_windows; i++)
    {
      if (window[i])
        {
          [self setWindowPos:window[i]];
          [window[i] makeKeyAndOrderFront:self];
        }
    }
}

- (void)setWindowPos:(NSWindow *)plotWindow
{
  cascadingPoint = [plotWindow cascadeTopLeftFromPoint:cascadingPoint];
}

- (void)keepOnDisplay:(NSNotification *)aNotification
{
  GKSView *tmpView = [aNotification object];
  int win = [tmpView getWinID];
  close_window[win] = NO;
}

- (int)getNextWindowID
{
  /* Search unused window */
  int unused_win_id;
  for (unused_win_id = 0; unused_win_id < MAX_WINDOWS && window[unused_win_id]; unused_win_id++)
    ;

  /* Either return the index of an unused window or MAX_WINDOWS */
  return unused_win_id;
}

@end
