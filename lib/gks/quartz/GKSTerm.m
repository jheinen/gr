
#include "gks.h"
#include "gkscore.h"
#include "gksquartz.h"
#include "zmq.h"

#import "GKSTerm.h"
#import "GKSView.h"

@interface GKSNetworkingForwarderThread : NSObject
+ (void)run:(GKSTerm *)gksterm;
@end

@interface GKSNetworkingWorkerThread : NSObject
+ (void)run:(GKSTerm *)gksterm;
@end

static void *context = NULL;

static void send_message(void *socket, void *data, size_t data_len)
{
  zmq_msg_t message;
  zmq_msg_init_size(&message, data_len);
  memcpy(zmq_msg_data(&message), data, data_len);
  zmq_msg_send(&message, socket, 0);
  zmq_msg_close(&message);
}

static void handle_create_window(GKSTerm *gksterm, void *socket, unsigned char *data)
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
  reply[0] = GKSTERM_FUNCTION_CREATE_WINDOW;
  *(int *)(reply + 1) = result;
  send_message(socket, reply, sizeof(reply));

  /* Show the app icon in dock */
  ProcessSerialNumber psn = {0, kCurrentProcess};
  TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}

static void handle_is_alive(GKSTerm *gksterm, void *socket, unsigned char *data)
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
  send_message(socket, reply, sizeof(reply));
}

static void handle_is_running(GKSTerm *gksterm, void *socket, unsigned char *data)
{
  (void)data;
  char reply[1];
  reply[0] = GKSTERM_FUNCTION_IS_RUNNING;
  send_message(socket, reply, sizeof(reply));
}

static void handle_draw(GKSTerm *gksterm, void *socket, unsigned char *data)
{
  // Send acknowledgement before actually drawing to avoid timeout
  char reply[1];
  reply[0] = GKSTERM_FUNCTION_DRAW;
  send_message(socket, reply, sizeof(reply));

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

static void handle_close_window(GKSTerm *gksterm, void *socket, unsigned char *data)
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
  send_message(socket, reply, sizeof(reply));
}

static void handle_get_state(GKSTerm *gksterm, void *socket, unsigned char *data)
{
  int window = *(int *)data;
  __block gks_ws_state_t state;
  dispatch_sync(dispatch_get_main_queue(), ^{
    @synchronized(gksterm)
    {
      state = [gksterm GKSQuartzGetState:window];
    }
  });
  char reply[1 + sizeof(gks_ws_state_t)];
  reply[0] = GKSTERM_FUNCTION_INQ_WS_STATE;
  memcpy(reply + 1, (void *)&state, sizeof(gks_ws_state_t));
  send_message(socket, reply, sizeof(reply));
}

static void handle_unknown(void *socket, unsigned char *data)
{
  (void)data;
  char reply[1];
  reply[0] = GKSTERM_FUNCTION_UNKNOWN;
  NSLog(@"ZeroMQ message with unknown function code");
  send_message(socket, reply, sizeof(reply));
}

static void handle_message(GKSTerm *gksterm, void *socket)
{
  zmq_msg_t message;
  zmq_msg_init(&message);
  zmq_msg_recv(&message, socket, 0);
  unsigned char *data = (unsigned char *)zmq_msg_data(&message);
  switch (data[0])
    {
    case GKSTERM_FUNCTION_CREATE_WINDOW:
      handle_create_window(gksterm, socket, data + 1);
      break;
    case GKSTERM_FUNCTION_DRAW:
      handle_draw(gksterm, socket, data + 1);
      break;
    case GKSTERM_FUNCTION_IS_ALIVE:
      handle_is_alive(gksterm, socket, data + 1);
      break;
    case GKSTERM_FUNCTION_CLOSE_WINDOW:
      handle_close_window(gksterm, socket, data + 1);
      break;
    case GKSTERM_FUNCTION_IS_RUNNING:
      handle_is_running(gksterm, socket, data + 1);
      break;
    case GKSTERM_FUNCTION_INQ_WS_STATE:
      handle_get_state(gksterm, socket, data + 1);
      break;
    default:
      handle_unknown(socket, data + 1);
      break;
    }
  zmq_msg_close(&message);
}

static void forward_message(void *input_socket, void *output_socket)
{
  // Forward a multipart message from one zeromq socket to another.
  zmq_msg_t messages[3];
  int more = 1;
  int num_parts = 0;
  for (int part = 0; part < 3 && more; part++)
    {
      zmq_msg_init(&messages[part]);
      zmq_msg_recv(&messages[part], input_socket, 0);
      more = zmq_msg_more(&messages[part]);
      num_parts++;
    }
  // Return IS_RUNNING messages to ROUTER
  if (zmq_msg_size(&messages[num_parts - 1]) > 0)
    {
      unsigned char *data = (unsigned char *)zmq_msg_data(&messages[num_parts - 1]);
      if (data[0] == GKSTERM_FUNCTION_IS_RUNNING)
        {
          output_socket = input_socket;
        }
    }
  for (int part = 0; part < num_parts; part++)
    {
      zmq_msg_send(&messages[part], output_socket, (part + 1 < num_parts) ? ZMQ_SNDMORE : 0);
      zmq_msg_close(&messages[part]);
    }
}

@implementation GKSNetworkingForwarderThread
+ (void)run:(GKSTerm *)gksterm
{
  // Handle requests incoming via ZeroMQ
  void *frontend = zmq_socket(context, ZMQ_ROUTER);
  void *backend = zmq_socket(context, ZMQ_DEALER);
  zmq_bind(frontend, "ipc:///tmp/GKSTerm.sock");
  zmq_bind(backend, "inproc://:gksterm:");

  zmq_pollitem_t items[] = {{frontend, 0, ZMQ_POLLIN, 0}, {backend, 0, ZMQ_POLLIN, 0}};
  while (YES)
    {
      zmq_poll(items, 2, -1);
      if (items[0].revents & ZMQ_POLLIN)
        {
          forward_message(frontend, backend);
        }
      if (items[1].revents & ZMQ_POLLIN)
        {
          forward_message(backend, frontend);
        }
    }
  zmq_close(frontend);
  zmq_close(backend);
}
@end


@implementation GKSNetworkingWorkerThread
+ (void)run:(GKSTerm *)gksterm
{
  void *worker = zmq_socket(context, ZMQ_REP);
  zmq_connect(worker, "inproc://:gksterm:");

  zmq_pollitem_t items[] = {{worker, 0, ZMQ_POLLIN, 0}};
  while (YES)
    {
      zmq_poll(items, 1, -1);
      if (items[0].revents & ZMQ_POLLIN)
        {
          handle_message(gksterm, worker);
        }
    }
  zmq_close(worker);
}
@end


static bool initialized = NO;

@implementation GKSTerm

- (void)awakeFromNib
{
  int win;

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keepOnDisplay:)
                                               name:@"GKSViewKeepOnDisplayNotification"
                                             object:nil];

  if (!initialized)
    {
      // Start networking threads
      context = zmq_ctx_new();
      [NSThread detachNewThreadSelector:@selector(run:) toTarget:[GKSNetworkingForwarderThread class] withObject:self];
      [NSThread detachNewThreadSelector:@selector(run:) toTarget:[GKSNetworkingWorkerThread class] withObject:self];

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

- (gks_ws_state_t)GKSQuartzGetState:(int)win
{
  __block gks_ws_state_t state;
  if (view[win])
    {
      CGSize size = view[win].bounds.size;

      state.width = size.width;
      state.height = size.height;
    }
  else
    {
      state.width = 500;
      state.height = 500;
    }
  if (window[win])
    {
      state.device_pixel_ratio = [window[win] backingScaleFactor];
    }
  else if ([NSScreen mainScreen] != nil)
    {
      state.device_pixel_ratio = [[NSScreen mainScreen] backingScaleFactor];
    }
  else
    {
      state.device_pixel_ratio = 1.0;
    }

  return state;
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
