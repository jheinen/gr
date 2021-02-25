JSTerm = function(ispluto=false) {
  if (typeof grJSTermRunning === 'undefined' || !grJSTermRunning) {
    BOXZOOM_THRESHOLD = 3; // Minimal size in pixels of the boxzoom-box to trigger a boxzoom-event
    BOXZOOM_TRIGGER_THRESHHOLD = 1000; // Time to wait (in ms) before triggering boxzoom event instead
    // of panning when pressing the left mouse button without moving the mouse
    RECONNECT_PLOT_TIMEOUT = 100; // Time to wait between attempts to connect to a plot's canvas
    RECONNECT_PLOT_MAX_ATTEMPTS = 50; // Maximum number of canvas reconnection attempts
    BOXZOOM_FILL_STYLE = '#bed2e8'; // Fill style of the boxzoom box
    DEFAULT_WIDTH = 600;
    DEFAULT_HEIGHT = 450;
    CREATE_CANVAS_TIMEOUT = 5000;

    ARROW_SIZE = [8, 7];
    STYLE_CSS = `
      .jsterm-tooltip {
        background-color: rgba(255, 255, 255, 0.95);
        border: 1px solid #e5e5e5;
        padding: 3px;
        position: absolute;
        z-index: 1;
        display:none;
      }
      .jsterm-tooltip-label {
        color: #26aae1;
        font-size: 0.8em;
        line-height: 0.9;
      }
      .jsterm-tooltip-value {
        color: #3c3c3c;
        font-size: 0.8em;
        line-height: 0.9;
      }
      .jsterm-right-arrow {
        width: 0;
        height: 0;
        border-top: ` + ARROW_SIZE[0] + `px solid transparent;
        border-bottom: ` + ARROW_SIZE[0] + `px solid transparent;
        border-left: ` + ARROW_SIZE[1] + `px solid #909599;
        z-index: 1;
        position: absolute;
        display: none;
      }
      .jsterm-left-arrow {
        width: 0;
        height: 0;
        border-top: ` + ARROW_SIZE[0] + `px solid transparent;
        border-bottom: ` + ARROW_SIZE[0] + `px solid transparent;
        border-right: ` + ARROW_SIZE[1] + `px solid #909599;
        z-index: 1;
        position: absolute;
        display: none;
      }
    `;

    //tooltip template
    TOOLTIP_DEFAULT_HTML_LABEL_SET =
      `<span class="jsterm-tooltip-label">{$label}</span><br>
      <span class="jsterm-tooltip-label">{$xlabel}: </span>
      <span class="jsterm-tooltip-value">{$x}</span><br>
      <span class="jsterm-tooltip-label">{$ylabel}</span>
      <span class="jsterm-tooltip-value">{$y}</span>`;
    TOOLTIP_DEFAULT_HTML_LABEL_NOT_SET =
      `<span class="jsterm-tooltip-label">{$xlabel}: </span>
      <span class="jsterm-tooltip-value">{$x}</span><br>
      <span class="jsterm-tooltip-label">{$ylabel}</span>
      <span class="jsterm-tooltip-value">{$y}</span>`;
    TOOLTIP_MISSING_VALUE_REPLACEMENT = '[n.d.]';

    var grm, ws, widgets = {},
      wsOpen = false,
      scheduled_merges = [],
      references = {},
      ref_id = null;
    var display = [],
      widgets_to_save = new Set(),
      data_loaded = false,
      prev_id = -1;
    var next_anchor = null;

    var jsterm_ispluto = ispluto;

    /**
     * Sends a mouse-event via websocket
     * @param  {Object} data Data describing the event
     * @param  {string} id   Identifier of the calling plot
     */
    sendEvt = function(data, id) {
      if (wsOpen) {
        ws.send(JSON.stringify({
          "type": "evt",
          "content": data,
          "id": id
        }));
      }
    };

    sendAck = function(id) {
      if (wsOpen) {
        ws.send(JSON.stringify({
          "type": "ack",
          "dispid": id
        }));
      }
    };

    createDisplay = function(id) {
      if (wsOpen) {
        ws.send(JSON.stringify({
          "type": "createDisplay",
          "dispid": id
        }));
      }
    };

    sendValue = function(key, value) {
      if (wsOpen) {
        ws.send(JSON.stringify({
          "type": "value",
          "key": key,
          "value": value
        }));
      }
    };

    /**
     * Creates a canvas to display a JSTermWidget
     * @param  {JSTermWidget} widget The widget to be displayed
     */
    createCanvas = function(widget, msg_sent = false) {
      let disp = document.getElementById('jsterm-display-' + widget.display);
      if (jsterm_ispluto) {
        disp.innerHTML = "";
      }
      if (disp === null) {
        if (wsOpen) {
          // TODO: Wenn ungültiges Canvas übergeben wird löst dies ein endlose rekursion aus
          if (display.length > 0 && !msg_sent) {
            widget.display = display[0];
            createDisplay(widget.display);
          }
          window.setTimeout(function() {
            createCanvas(widget, msg_sent = true);
          }, CREATE_CANVAS_TIMEOUT);
        } else {
          disp = document.createElement('div');
          disp.id = 'jsterm-display-' + widget.id;
          var anchor = null;
          if (next_anchor !== null) {
            anchor = document.getElementById(next_anchor);
            next_anchor = null;
          }
          if (anchor === null) {
            anchor = document.body;
          }
          anchor.appendChild(disp);
          widget.display = "display-" + disp.id;
        }
      }
      if (disp !== null) {
        disp.style = "display: inline;";
        let div = document.createElement('div');
        div.id = 'jsterm-div-' + widget.id;
        div.style = 'position: relative; width:' + widget.width + 'px; height: ' + widget.height + 'px;';
        let overlay = document.createElement('canvas');
        overlay.id = 'jsterm-overlay-' + widget.id;
        overlay.style = 'position:absolute; top: 0; right: 0; z-index: 2;';
        overlay.width = widget.width;
        overlay.height = widget.height;
        let canvas = document.createElement('canvas');
        canvas.id = 'jsterm-' + widget.id;
        canvas.style = 'position: absolute; top: 0; right: 0; z-index: 0';
        canvas.width = widget.width;
        canvas.height = widget.height;
        div.appendChild(overlay);
        div.appendChild(canvas);
        disp.appendChild(div);
        widget.connectCanvas();
        widget.draw();
      }
    };

    /**
     * Sends a save-event via websocket
     */
    saveData = function(data, plot_id, display, width, height, tooltip) {
      if (wsOpen) {
        ws.send(JSON.stringify({
          "type": "save",
          "display_id": display,
          "content": {
            "data": {
              "widget_data": `
                if (typeof storedData === 'undefined') {
                  var storedData = [];
                }
                storedData.push({
                  "timestamp": ` + Date.now() + `,
                  "width": ` + width + `,
                  "height": ` + height + `,
                  "tooltip": {
                    "html": "` + tooltip.html + `",
                    "data": ` + JSON.stringify(tooltip.data) + `
                  },
                  "display_id": "` + display + `",
                  "plot_id": ` + plot_id + `,
                  "grm": ` + JSON.stringify(data) + `
                });
              `
            }
          }
        }));
      }
    };

    /**
     * Establishes the websocket connection
     */
    this.connectWs = function() {
      if (!GR.is_ready) {
        GR.ready(function() {
          return this.connectWs();
        }.bind(this));
        return;
      }
      if (typeof WEB_SOCKET_ADDRESS === 'undefined') {
        WEB_SOCKET_ADDRESS = 'ws://127.0.0.1:8081'
      }
      ws = new WebSocket(WEB_SOCKET_ADDRESS);
      ws.onerror = function(e) {
        wsOpen = false;
      };
      ws.onmessage = function(msg) {
        let data = JSON.parse(msg.data);
        if (data.type === 'evt') {
          if (typeof references[data.id] !== 'undefined') {
            references[data.id].msgHandleEvent(data);
          }
        } else if (data.type === 'cmd') {
          if (typeof data.id !== 'undefined') {
            if (typeof references[data.id] !== 'undefined') {
              references[data.id].msgHandleCommand(data);
            }
          } else {
            for (let key in references) {
              references[key].msgHandleCommand(data);
            }
          }
        } else if (data.type === 'inq') {
          switch (data.value) {
            case "prev_id":
              sendValue("prev_id", prev_id);
              break;
          }
        } else if (data.type === 'set_ref_id') {
          ref_id = data.id;
        } else if (data.type === 'draw') {
          this.draw(data);
        }
      }.bind(this);
      ws.onclose = function() {
        wsOpen = false;
      };
      ws.onopen = function() {
        wsOpen = true;
        ws.send('js-running');
      };
      window.addEventListener('beforeunload', function(e) {
        ws.close();
      });
    };

    /**
     * Return the JSTerm GRM instance.
     */
    this.grmInstance = function() {
      return grm;
    };

    /**
     * Set an HTML element id as the anchor for the next JSTermWidget
     * @param  {string} anchor   Id of the HTML anchor
     */
    this.nextWidgetAnchor = function(anchor) {
      next_anchor = anchor;
    };

    /**
     * Handles a draw command.
     * @param  {[type]} msg The input message containing the draw command
     */
    this.draw = function(msg) {
      if (!GR.is_ready) {
        GR.ready(function() {
          return this.draw(msg);
        }.bind(this));
        return;
      }
      if (typeof grm === 'undefined') {
        onLoad();
      }
      let args = grm.args_new();
      grm.read(args, msg.json.replace(/\"/g, '"'));
      display.push(msg.display);
      grm.merge_named(args, "jstermMerge" + msg.display);
      grm.args_delete(args);
    };

    /**
     * Draw data that has been saved in the loaded page
     */
    drawSavedData = function() {
      if (data_loaded) {
        return;
      }
      if (typeof storedData === 'undefined') {
        return;
      }
      data_loaded = true;
      let created_widgets = [];
      let timestamps = {};
      for (let i = 0; i < storedData.length; i++) {
        let widget_data = storedData[i];
        if (typeof timestamps[widget_data.plot_id] === 'undefined' || widget_data.timestamp < timestamps[widget_data.plot_id]) {
          timestamps[widget_data.plot_id] = widget_data.timestamp;
          widgets[widget_data.plot_id] = new JSTermWidget(widget_data.plot_id);
          widgets[widget_data.plot_id].display = widget_data.display_id;
          widgets[widget_data.plot_id].width = widget_data.width;
          widgets[widget_data.plot_id].height = widget_data.height;
          widgets[widget_data.plot_id].tooltip.html = widget_data.tooltip.html;
          widgets[widget_data.plot_id].tooltip.data = widget_data.tooltip.data;
          // TODO: Das hier erst am Schluss machen, wenn klar ist, dass keine aktuelleren Daten gefunden wurden
          createCanvas(widgets[widget_data.plot_id]);
          grm.switch(widget_data.plot_id);
          let data = grm.load_from_str(widget_data.grm);
          widgets[widget_data.plot_id].draw();
        } else {
          // TODO
          console.log('older widget data for plot ID', widget_data.plot_id, 'found');
        }
      }
    };

    /**
     * Creates a JSTermWidget-Object describing and managing a canvas
     * @param       {number} id     The widget's numerical identifier (belonging context in `grm.c`)
     * @constructor
     */
    JSTermWidget = function(id) {
      this.id = id; // context id for grm.c (switch)

      /**
       * Initialize the JSTermWidget
       */
      this.init = function() {
        this.canvas = undefined;
        this.overlayCanvas = undefined;
        this.div = undefined;

        this.waiting = false;

        // event handling
        this.pinching = false;
        this.panning = false;
        this.prevMousePos = undefined;
        this.boxzoom = false;
        this.keepAspectRatio = true;
        this.boxzoomTriggerTimeout = undefined;
        this.boxzoomPoint = [undefined, undefined];
        this.pinchDiff = 0;
        this.prevTouches = undefined;

        this.sendEvents = false;
        this.handleEvents = true;

        this.display = undefined;

        this.width = DEFAULT_WIDTH;
        this.height = DEFAULT_HEIGHT;
        this.ref_id = null;

        this.tooltip = {
          "html": "",
          "data": {}
        };
      };

      this.init();

      /**
       * Resizes the JSTermWidget
       * @param  {number} height new canvas height in pixels
       */
      this.resize = function(width, height) {
        if (width != this.width || height != this.height) {
          this.width = width;
          this.height = height;
          if (this.canvas !== undefined) {
            this.canvas.width = width;
            this.canvas.height = height;
            this.overlayCanvas.width = width;
            this.overlayCanvas.height = height;
            this.div.style = "position: relative; width: " + width + "px; height: " + height + "px;";
          }
          this.draw();
          this.save();
        }
      };

      /**
       * Send an event fired by widget via websocket
       * @param  {Object} data Event description
       */
      this.sendEvt = function(data) {
        if (wsOpen && this.sendEvents && this.ref_id !== null) {
          sendEvt(data, this.ref_id);
        }
      };

      /**
       * Calculate the position of the mouse on the canvas in pixels,
       * relative to the upper left corner.
       * @param  {Event} event    The mouse event to process
       * @return {[number, number]}       The calculated [x, y]-coordinates
       */
      this.getCoords = function(event) {
        let rect = this.canvas.getBoundingClientRect();
        //TODO mind the canvas-padding if necessary!
        return [Math.floor(event.clientX - rect.left), Math.floor(event.clientY - rect.top)];
      };

      /**
       * Send an event to the GRM runtime
       * @param  {number} mouseargs (Emscripten) address of the argumentcontainer describing an event
       */
      this.grEventinput = function(mouseargs) {
        grm.switch(this.id);
        grm.input(mouseargs);
        grm.current_canvas = this.canvas;
        grm.current_context = grm.current_canvas.getContext('2d');
        grm.select_canvas();
        grm.plot();
      };

      /**
       * Handles a wheel event (zoom)
       * @param  {number} x       x-coordinate on the canvas of the mouse
       * @param  {number} y       y-coordinate on the canvas of the mouse
       * @param  {number} angle_delta angle the wheel has been turned
       */
      this.handleWheel = function(x, y, angle_delta) {
        let context = this.overlayCanvas.getContext('2d');
        context.clearRect(0, 0, this.overlayCanvas.width, this.overlayCanvas.height);

        if (typeof this.boxzoomTriggerTimeout !== 'undefined') {
          clearTimeout(this.boxzoomTriggerTimeout);
        }
        let mouseargs = grm.args_new();
        grm.args_push(mouseargs, "x", "i", [x]);
        grm.args_push(mouseargs, "y", "i", [y]);
        grm.args_push(mouseargs, "angle_delta", "d", [angle_delta]);
        this.grEventinput(mouseargs);
        grm.args_delete(mouseargs);
      };

      /**
       * Handles a wheel event triggered by the mouse
       * @param  {Event} event The fired mouse event
       */
      this.mouseHandleWheel = function(event) {
        let coords = this.getCoords(event);
        this.sendEvt({
          "x": coords[0],
          "y": coords[1],
          "angle_delta": event.deltaY,
          "event": "mousewheel",
        });
        if (this.handleEvents) {
          this.handleWheel(coords[0], coords[1], event.deltaY);
        }
        event.preventDefault();
      };

      /**
       * Handles a mousedown event
       * @param  {number} x       x-coordinate on the canvas of the mouse
       * @param  {number} y       y-coordinate on the canvas of the mouse
       * @param  {number} button  Integer indicating the button pressed (0: left, 1: middle/wheel, 2: right)
       * @param  {Boolean} ctrlKey Boolean indicating if the ctrl-key is pressed
       */
      this.handleMouseDown = function(x, y, button, ctrlKey) {
        if (typeof this.boxzoomTriggerTimeout !== 'undefined') {
          clearTimeout(this.boxzoomTriggerTimeout);
        }
        if (button == 0) {
          this.overlayCanvas.style.cursor = 'move';
          this.panning = true;
          this.boxzoom = false;
          this.prevMousePos = [x, y];
          this.boxzoomTriggerTimeout = setTimeout(function() {
            this.startBoxzoom(x, y, ctrlKey);
          }.bind(this), BOXZOOM_TRIGGER_THRESHHOLD);
        } else if (button == 2) {
          this.startBoxzoom(x, y, ctrlKey);
        }
      };

      /**
       * Handles a mousedown event triggered by the mouse
       * @param  {Event} event The fired mouse event
       */
      this.mouseHandleMouseDown = function(event) {
        let coords = this.getCoords(event);
        this.sendEvt({
          "x": coords[0],
          "y": coords[1],
          "button": event.button,
          "ctrlKey": event.ctrlKey,
          "event": "mousedown",
        });
        if (this.handleEvents) {
          this.handleMouseDown(coords[0], coords[1], event.button, event.ctrlKey);
        }
        event.preventDefault();
      };

      /**
       * Initiate the boxzoom on the canvas.
       * @param  {number} x       x-coordinate of the mouse
       * @param  {number} y       y-coordinate of the mouse
       * @param  {Boolean} ctrlKey Boolean indicating if the ctrl-key is pressed
       */
      this.startBoxzoom = function(x, y, ctrlKey) {
        this.panning = false;
        this.boxzoom = true;
        if (ctrlKey) {
          this.keepAspectRatio = false;
        }
        this.boxzoomPoint = [x, y];
        this.overlayCanvas.style.cursor = 'nwse-resize';
      };

      /**
       * Handles a mouseup event
       * @param  {number} x       x-coordinate on the canvas of the mouse
       * @param  {number} y       y-coordinate on the canvas of the mouse
       * @param  {number} button  Integer indicating the button pressed (0: left, 1: middle/wheel, 2: right)
       */
      this.handleMouseUp = function(x, y, button) {
        if (typeof this.boxzoomTriggerTimeout !== 'undefined') {
          clearTimeout(this.boxzoomTriggerTimeout);
        }
        if (this.boxzoom) {
          if ((Math.abs(this.boxzoomPoint[0] - x) >= BOXZOOM_THRESHOLD) && (Math.abs(this.boxzoomPoint[1] - y) >= BOXZOOM_THRESHOLD)) {
            let mouseargs = grm.args_new();
            let diff = [x - this.boxzoomPoint[0], y - this.boxzoomPoint[1]];
            grm.args_push(mouseargs, "x1", "i", [this.boxzoomPoint[0]]);
            grm.args_push(mouseargs, "x2", "i", [this.boxzoomPoint[0] + diff[0]]);
            grm.args_push(mouseargs, "y1", "i", [this.boxzoomPoint[1]]);
            grm.args_push(mouseargs, "y2", "i", [this.boxzoomPoint[1] + diff[1]]);
            if (this.keepAspectRatio) {
              grm.args_push(mouseargs, "keep_aspect_ratio", "i", [1]);
            } else {
              grm.args_push(mouseargs, "keep_aspect_ratio", "i", [0]);
            }
            this.grEventinput(mouseargs);
            grm.args_delete(mouseargs);
          }
        }
        this.prevMousePos = undefined;
        this.overlayCanvas.style.cursor = 'auto';
        this.panning = false;
        this.boxzoom = false;
        this.boxzoomPoint = [undefined, undefined];
        this.keepAspectRatio = true;
        let context = this.overlayCanvas.getContext('2d');
        context.clearRect(0, 0, this.overlayCanvas.width, this.overlayCanvas.height);
      };

      /**
       * Handles a mouseup event triggered by the mouse
       * @param  {Event} event The fired mouse event
       */
      this.mouseHandleMouseUp = function(event) {
        let coords = this.getCoords(event);
        this.sendEvt({
          "x": coords[0],
          "y": coords[1],
          "button": event.button,
          "event": "mouseup",
        });
        if (this.handleEvents) {
          this.handleMouseUp(coords[0], coords[1], event.button);
        }
        event.preventDefault();
      };

      /**
       * Handles a touchstart event triggered by tapping the touchscreen
       * @param  {Event} event The fired touch event
       */
      this.touchHandleTouchStart = function(event) {
        if (event.touches.length == 1) {
          let coords = this.getCoords(event.touches[0]);
          this.handleMouseDown(coords[0], coords[1], 0, false);
        } else if (event.touches.length == 2) {
          this.pinching = true;
          this.pinchDiff = Math.abs(event.touches[0].clientX - event.touches[1].clientX) + Math.abs(event.touches[0].clientY - event.touches[1].clientY);
          let c1 = this.getCoords(event.touches[0]);
          let c2 = this.getCoords(event.touches[1]);
          this.prevTouches = [c1, c2];
        } else if (event.touches.length == 3) {
          let coords1 = this.getCoords(event.touches[0]);
          let coords2 = this.getCoords(event.touches[1]);
          let coords3 = this.getCoords(event.touches[2]);
          let x = 1 / 3 * (coords1[0] + coords2[0] + coords3[0]);
          let y = 1 / 3 * (coords1[1] + coords2[1] + coords3[1]);
          this.handleDoubleclick(x, y);
        }
        event.preventDefault();
      };

      /**
       * Handles a touchend event
       * @param  {Event} event The fired touch event
       */
      this.touchHandleTouchEnd = function(event) {
        this.handleMouseleave();
      };

      /**
       * Handles a touchmove event triggered by moving fingers on the touchscreen
       * @param  {Event} event The fired touch event
       */
      this.touchHandleTouchmove = function(event) {
        if (event.touches.length == 1) {
          let coords = this.getCoords(event.touches[0]);
          this.handleMouseMove(coords[0], coords[1]);
        } else if (this.pinching && event.touches.length == 2) {
          let c1 = this.getCoords(event.touches[0]);
          let c2 = this.getCoords(event.touches[1]);
          let diff = Math.sqrt(Math.pow(Math.abs(c1[0] - c2[0]), 2) + Math.pow(Math.abs(c1[1] - c2[1]), 2));
          if (typeof this.pinchDiff !== 'undefined' && typeof this.prevTouches !== 'undefined') {
            let factor = this.pinchDiff / diff;

            let mouseargs = grm.args_new();
            grm.args_push(mouseargs, "x", "i", [(c1[0] + c2[0]) / 2]);
            grm.args_push(mouseargs, "y", "i", [(c1[1] + c2[1]) / 2]);
            grm.args_push(mouseargs, "factor", "d", [factor]);
            this.grEventinput(mouseargs);
            grm.args_delete(mouseargs);

            let panmouseargs = grm.args_new();
            grm.args_push(panmouseargs, "x", "i", [(c1[0] + c2[0]) / 2]);
            grm.args_push(panmouseargs, "y", "i", [(c1[1] + c2[1]) / 2]);
            grm.args_push(panmouseargs, "xshift", "i", [(c1[0] - this.prevTouches[0][0] + c2[0] - this.prevTouches[1][0]) / 2.0]);
            grm.args_push(panmouseargs, "yshift", "i", [(c1[1] - this.prevTouches[0][1] + c2[1] - this.prevTouches[1][1]) / 2.0]);
            this.grEventinput(panmouseargs);
            grm.args_delete(panmouseargs);
          }
          this.pinchDiff = diff;
          this.prevTouches = [c1, c2];
        }
        event.preventDefault();
      };

      /**
       * Handles a mouseleave event
       */
      this.handleMouseleave = function() {
        if (typeof this.boxzoomTriggerTimeout !== 'undefined') {
          clearTimeout(this.boxzoomTriggerTimeout);
        }
        this.overlayCanvas.style.cursor = 'auto';
        this.panning = false;
        this.prevMousePos = undefined;
        if (this.boxzoom) {
          let context = this.overlayCanvas.getContext('2d');
          context.clearRect(0, 0, this.overlayCanvas.width, this.overlayCanvas.height);
        }
        this.tooltipDiv.innerHTML = "";
        this.tooltipDiv.style.display = 'none';
        this.overlayArrowLeft.style.display = 'none';
        this.overlayArrowRight.style.display = 'none';
        this.boxzoom = false;
        this.boxzoomPoint = [undefined, undefined];
        this.keepAspectRatio = true;
      };

      /**
       * Handles a mouseleave event triggered by the mouse
       * @param  {Event} event The fired mouse event
       */
      this.mouseHandleMouseleave = function(event) {
        this.pinchDiff = undefined;
        this.prevTouches = undefined;
        this.sendEvt({
          "event": "mouseleave",
        });
        if (this.handleEvents) {
          this.handleMouseleave();
        }
      };

      /**
       * Handles a mousemove event
       * @param  {number} x       x-coordinate on the canvas of the mouse
       * @param  {number} y       y-coordinate on the canvas of the mouse
       */
      this.handleMouseMove = function(x, y) {
        if (this.panning) {
          this.tooltipDiv.innerHTML = "";
          this.tooltipDiv.style.display = 'none';
          this.overlayArrowLeft.style.display = 'none';
          this.overlayArrowRight.style.display = 'none';
          let context = this.overlayCanvas.getContext('2d');
          context.clearRect(0, 0, this.overlayCanvas.width, this.overlayCanvas.height);
          if (typeof this.boxzoomTriggerTimeout !== 'undefined') {
            clearTimeout(this.boxzoomTriggerTimeout);
          }
          let mouseargs = grm.args_new();
          grm.args_push(mouseargs, "x", "i", [this.prevMousePos[0]]);
          grm.args_push(mouseargs, "y", "i", [this.prevMousePos[1]]);
          grm.args_push(mouseargs, "xshift", "i", [x - this.prevMousePos[0]]);
          grm.args_push(mouseargs, "yshift", "i", [y - this.prevMousePos[1]]);
          this.grEventinput(mouseargs);
          grm.args_delete(mouseargs);
          this.prevMousePos = [x, y];
        } else if (this.boxzoom) {
          this.tooltipDiv.innerHTML = "";
          this.tooltipDiv.style.display = 'none';
          this.overlayArrowLeft.style.display = 'none';
          this.overlayArrowRight.style.display = 'none';
          let context = this.overlayCanvas.getContext('2d');
          let diff = [x - this.boxzoomPoint[0], y - this.boxzoomPoint[1]];
          grm.switch(this.id);
          let box = grm.get_box(this.boxzoomPoint[0], this.boxzoomPoint[1], this.boxzoomPoint[0] + diff[0], this.boxzoomPoint[1] + diff[1], this.keepAspectRatio);
          context.clearRect(0, 0, this.overlayCanvas.width, this.overlayCanvas.height);
          if (diff[0] * diff[1] >= 0) {
            this.overlayCanvas.style.cursor = 'nwse-resize';
          } else {
            this.overlayCanvas.style.cursor = 'nesw-resize';
          }
          context.fillStyle = BOXZOOM_FILL_STYLE;
          context.beginPath();
          context.rect(box[0], box[1], box[2], box[3]);
          context.globalAlpha = 0.5;
          context.fill();
          context.closePath();
        } else {
          grm.switch(this.id);
          let tooltipInfo = grm.get_tooltip(x, y);
          if (tooltipInfo.xpx >= 0 && tooltipInfo.ypx >= 0) {
            let text;
            tooltipInfo.x = Math.round((tooltipInfo.x + Number.EPSILON) * 100) / 100;
            tooltipInfo.y = Math.round((tooltipInfo.y + Number.EPSILON) * 100) / 100;
            if (typeof this.tooltip === 'undefined' || this.tooltip.html == "") {
              if (tooltipInfo.label != "") {
                text = TOOLTIP_DEFAULT_HTML_LABEL_SET;
              } else {
                text = TOOLTIP_DEFAULT_HTML_LABEL_NOT_SET;
              }
            } else {
              text = this.tooltip.html;
            }
            let index = 0,
              start, end, key, substr, arrStart, arrEnd, subkey, replacement;
            start = text.indexOf('{$', index);
            while (start != -1) {
              end = text.indexOf('}', start);
              key = text.substring(start + 2, end);
              if (typeof tooltipInfo[key] !== 'undefined') {
                replacement = tooltipInfo[key];
              } else {
                replacement = TOOLTIP_MISSING_VALUE_REPLACEMENT;
              }
              text = text.substring(0, start) + replacement + text.substring(end + 1);
              index = index - (end - start) + tooltipInfo[key].length;
              start = text.indexOf('{$', index);
            }
            index = 0;
            start = text.indexOf('{@', index);
            while (start != -1) {
              end = text.indexOf('}', start);
              substr = text.substring(start + 2, end);
              arrStart = substr.indexOf('[');
              arrEnd = substr.indexOf(']');
              key = substr.substring(0, arrStart);
              subkey = substr.substring(arrStart + 1, arrEnd);
              if (subkey[0] == '$') {
                subkey = tooltipInfo[subkey.substring(1)];
              }
              if (typeof this.tooltip.data[key] !== 'undefined' && typeof this.tooltip.data[key][subkey] !== 'undefined') {
                replacement = this.tooltip.data[key][subkey];
              } else {
                replacement = TOOLTIP_MISSING_VALUE_REPLACEMENT;
              }
              text = text.substring(0, start) + replacement + text.substring(end + 1);
              index = index - (end - start) + replacement.length;
              start = text.indexOf('{@', index);
            }
            this.tooltipDiv.innerHTML = text;
            if (tooltipInfo.xpx > this.overlayCanvas.width / 2.0) {
              this.tooltipDiv.style.right = (this.overlayCanvas.width - tooltipInfo.xpx + ARROW_SIZE[1]) + 'px';
              this.tooltipDiv.style.left = 'auto';
              this.tooltipDiv.style.top = (tooltipInfo.ypx - 0.5 * this.tooltipDiv.clientHeight) + 'px';
              this.overlayArrowRight.style.right = (this.overlayCanvas.width - tooltipInfo.xpx) + 'px';
              this.overlayArrowRight.style.top = (tooltipInfo.ypx - ARROW_SIZE[1]) + 'px';
              this.overlayArrowRight.style.display = 'block';
              this.overlayArrowLeft.style.display = 'none';
            } else {
              this.tooltipDiv.style.left = (tooltipInfo.xpx + ARROW_SIZE[1]) + 'px';
              this.tooltipDiv.style.right = 'auto';
              this.tooltipDiv.style.top = (tooltipInfo.ypx - 0.5 * this.tooltipDiv.clientHeight) + 'px';
              this.overlayArrowLeft.style.left = tooltipInfo.xpx + 'px';
              this.overlayArrowLeft.style.top = (tooltipInfo.ypx - ARROW_SIZE[1]) + 'px';
              this.overlayArrowLeft.style.display = 'block';
              this.overlayArrowRight.style.display = 'none';
            }
            this.tooltipDiv.style.display = 'block';
          } else {
            this.tooltipDiv.innerHTML = "";
            this.tooltipDiv.style.display = 'none';
            this.overlayArrowLeft.style.display = 'none';
            this.overlayArrowRight.style.display = 'none';
          }
        }
      };

      /**
       * Handles a mousemove event triggered by the mouse
       * @param  {Event} event The fired mouse event
       */
      this.mouseHandleMouseMove = function(event) {
        let coords = this.getCoords(event);
        this.sendEvt({
          "x": coords[0],
          "y": coords[1],
          "event": "mousemove",
        });
        if (this.handleEvents) {
          this.handleMouseMove(coords[0], coords[1]);
        }
        event.preventDefault();
      };

      /**
       * Handles a doubleclick event
       * @param  {number} x       x-coordinate on the canvas of the mouse
       * @param  {number} y       y-coordinate on the canvas of the mouse
       */
      this.handleDoubleclick = function(x, y) {
        let mouseargs = grm.args_new();
        grm.args_push(mouseargs, "x", "i", [x]);
        grm.args_push(mouseargs, "y", "i", [y]);
        grm.args_push(mouseargs, "key", "s", "r");
        this.grEventinput(mouseargs);
        grm.args_delete(mouseargs);
        this.boxzoomPoint = [undefined, undefined];
      };

      /**
       * Handles a doubleclick event triggered by the mouse
       * @param  {Event} event The fired mouse event
       */
      this.mouseHandleDoubleclick = function(event) {
        let coords = this.getCoords(event);
        this.sendEvt({
          "x": coords[0],
          "y": coords[1],
          "event": "doubleclick",
        });
        if (this.handleEvents) {
          this.handleDoubleclick(coords[0], coords[1]);
        }
        event.preventDefault();
      };

      /**
       * Handles an event triggered by a Jupyter Comm message
       * @param  {Object} msg The message describing the event
       */
      this.msgHandleEvent = function(msg) {
        switch (msg.event) {
          case "mousewheel":
            this.handleWheel(msg.x, msg.y, msg.angle_delta);
            break;
          case "mousedown":
            this.handleMouseDown(msg.x, msg.y, msg.button, msg.ctrlKey);
            break;
          case "mouseup":
            this.handleMouseUp(msg.x, msg.y, msg.button);
            break;
          case "mousemove":
            this.handleMouseMove(msg.x, msg.y);
            break;
          case "doubleclick":
            this.handleDoubleclick(msg.x, msg.y);
            break;
          case "mouseleave":
            this.handleMouseleave();
            break;
          default:
            break;
        }
      };

      /**
       * Handles a command received via websocket
       * @param  {Object} msg Received msg containing the command
       */
      this.msgHandleCommand = function(msg) {
        switch (msg.command) {
          case 'enable_events':
            this.sendEvents = true;
            break;
          case 'disable_events':
            this.sendEvents = false;
            break;
          case 'enable_jseventhandling':
            this.handleEvents = true;
            break;
          case 'disable_jseventhandling':
            this.handleEvents = false;
            break;
          case 'settooltip':
            this.tooltip.html = msg.html;
            this.tooltip.data = msg.data;
            this.save();
            break;
          default:
            break;
        }
      };

      /**
       * Draw a plot described by a message received via websocket
       * @param  {Object} msg message containing the draw-command
       */
      this.draw = function() {
        if (typeof this.display === 'undefined' || document.getElementById('jsterm-' + this.id) == null) {
          this.canvas = undefined;
          this.display = display[0];
          return createCanvas(this);
        }
        if (document.getElementById('jsterm-' + this.id) !== this.canvas || typeof this.canvas === 'undefined' || typeof this.overlayCanvas === 'undefined') {
          this.connectCanvas();
        }

        grm.switch(this.id);
        grm.current_canvas = this.canvas;
        grm.current_context = grm.current_canvas.getContext('2d');
        grm.select_canvas();
        grm.plot();
      };

      /**
       * Connects a canvas to a JSTermWidget object.
       */
      this.connectCanvas = function() {
        if (document.getElementById('jsterm-' + this.id) != null) {
          this.div = document.getElementById('jsterm-div-' + this.id);
          this.div.style.position = 'relative';
          this.canvas = document.getElementById('jsterm-' + this.id);
          this.overlayCanvas = document.getElementById('jsterm-overlay-' + this.id);
          this.overlayCanvas.style.cursor = 'auto';
          this.tooltipDiv = document.createElement('div');
          this.tooltipDiv.classList.add('jsterm-tooltip');
          this.tooltipDiv.innerHTML = '';
          this.div.appendChild(this.tooltipDiv);

          this.overlayArrowLeft = document.createElement('div');
          this.overlayArrowLeft.classList.add('jsterm-left-arrow');
          this.div.appendChild(this.overlayArrowLeft);

          this.overlayArrowRight = document.createElement('div');
          this.overlayArrowRight.classList.add('jsterm-right-arrow');
          this.div.appendChild(this.overlayArrowRight);

          //registering event handler
          this.overlayCanvas.addEventListener('wheel', function(evt) {
            this.mouseHandleWheel(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('mousedown', function(evt) {
            this.mouseHandleMouseDown(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('touchstart', function(evt) {
            this.touchHandleTouchStart(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('touchmove', function(evt) {
            this.touchHandleTouchmove(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('touchend', function(evt) {
            this.touchHandleTouchEnd(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('mousemove', function(evt) {
            this.mouseHandleMouseMove(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('mouseup', function(evt) {
            this.mouseHandleMouseUp(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('mouseleave', function(evt) {
            this.mouseHandleMouseleave(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('dblclick', function(evt) {
            this.mouseHandleDoubleclick(evt);
          }.bind(this));
          this.overlayCanvas.addEventListener('contextmenu', function(event) {
            event.preventDefault();
            return false;
          });
        }
      };

      this.save = function() {
        if (wsOpen) {
          grm.switch(this.id);
          let data = grm.dump_json_str();
          saveData(data, this.id, this.display, this.width, this.height, this.tooltip);
        }
      };
    };

    /**
     * Callback for grm's size event. Handles event and resizes canvas if required.
     */
    sizeCallback = function(evt) {
      widgets[evt.plot_id].resize(evt.width, evt.height);
    };

    /**
     * Callback for grm's new plot event. Handles event and creates new canvas.
     */
    newPlotCallback = function(evt) {
      if (typeof widgets[evt.plot_id] === 'undefined') {
        widgets[evt.plot_id] = new JSTermWidget(evt.plot_id);
      }
      prev_id = evt.plot_id;
      if (ref_id != null) {
        if (typeof references[ref_id] !== 'undefined') {
          references[ref_id].ref_id = null;
        }
        references[ref_id] = widgets[evt.plot_id];
        references[ref_id].ref_id = ref_id;
        ref_id = null;
      }
      widgets_to_save.add(evt.plot_id);
    };

    /**
     * Callback for grm's update plot event. Handles event and creates canvas id needed.
     */
    updatePlotCallback = function(evt) {
      if (typeof widgets[evt.plot_id] === 'undefined') {
        console.error('Updated plot does not exist, creating new object. (id', evt.plot_id, ')');
        widgets[evt.plot_id] = new JSTermWidget(evt.plot_id);
      }
      widgets_to_save.add(evt.plot_id);
    };

    /**
     * Callback for grm's merge end event.
     * Acknowledge the finished execution of a `draw()` command.
     */
    mergeEndCallback = function(evt) {
      let display_uuid = evt.identificator.substring("jstermMerge".length);
      if (!wsOpen || display_uuid.length != 0) {
        iter = Array.from(widgets_to_save);
        for (let w in iter) {
          if (w != 'last') {
            widgets[iter[w]].draw();
            widgets[iter[w]].save();
          }
        }
        display.shift();
        widgets_to_save.clear();
        sendAck(display_uuid);
      }
    };

    /**
     * Function to call when page has been loaded.
     */
    onLoad = function() {
      if (!GR.is_ready) {
        GR.ready(function() {
          return onLoad();
        });
        return;
      } else {
        if (typeof grm === 'undefined') {
          let canvas = document.createElement('canvas');
          canvas.id = 'jsterm-hidden-canvas';
          canvas.width = 640;
          canvas.height = 480;
          canvas.style = 'display: none;';
          document.body.appendChild(canvas);
          grm = new GRM('jsterm-hidden-canvas');
          grm.register(grm.EVENT_SIZE, sizeCallback);
          grm.register(grm.EVENT_NEW_PLOT, newPlotCallback);
          grm.register(grm.EVENT_UPDATE_PLOT, updatePlotCallback);
          grm.register(grm.EVENT_MERGE_END, mergeEndCallback);
          let args = grm.args_new();
          grm.args_push(args, 'append_plots', 'i', [1]);
          grm.args_push(args, 'hold_plots', 'i', [1]);
          grm.merge(args);
          grm.args_delete(args);
        }
        if (document.getElementById('jsterm-style') == null) {
          let style = document.createElement('style');
          style.id = 'jsterm-style';
          style.textContent = STYLE_CSS;
          document.head.append(style);
        }
        if (!jsterm_ispluto) {
          drawSavedData();
        }
      }
    };

    if (document.readyState != 'loading') {
      onLoad();
    } else if (document.addEventListener) {
      document.addEventListener('DOMContentLoaded', onLoad);
    } else {
      document.attachEvent('onreadystatechange', function() {
        if (document.readyState == 'complete') {
          onLoad();
        }
      });
    }
  }
  var grJSTermRunning = true;
};
