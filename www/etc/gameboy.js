
function InitGameboy() {
    class JoystickController
    {
        // stickID: ID of HTML element (representing joystick) that will be dragged
        // maxDistance: maximum amount joystick can move in any direction
        // deadzone: joystick must move at least this amount from origin to register value change
        constructor( stickID, maxDistance, deadzone )
        {
            this.id = stickID;
            let stick = document.getElementById(stickID);
    
            // location from which drag begins, used to calculate offsets
            this.dragStart = null;
    
            // track touch identifier in case multiple joysticks present
            this.touchId = null;
    
            this.active = false;
            this.threshold = false;
            this.value = { x: 0, y: 0 };
    
            let self = this;
    
            function handleDown(event)
            {
                self.active = true;
    
                // all drag movements are instantaneous
                stick.style.transition = '0s';
    
                // touch event fired before mouse event; prevent redundant mouse event from firing
                event.preventDefault();
    
                if (event.changedTouches)
                    self.dragStart = { x: event.changedTouches[0].clientX, y: event.changedTouches[0].clientY };
                else
                    self.dragStart = { x: event.clientX, y: event.clientY };
    
                // if this is a touch event, keep track of which one
                if (event.changedTouches)
                    self.touchId = event.changedTouches[0].identifier;
            }
    
            function handleMove(event)
            {
                if ( !self.active ) return;
    
                // if this is a touch event, make sure it is the right one
                // also handle multiple simultaneous touchmove events
                let touchmoveId = null;
                if (event.changedTouches)
                {
                    for (let i = 0; i < event.changedTouches.length; i++)
                    {
                        if (self.touchId == event.changedTouches[i].identifier)
                        {
                            touchmoveId = i;
                            event.clientX = event.changedTouches[i].clientX;
                            event.clientY = event.changedTouches[i].clientY;
                        }
                    }
    
                    if (touchmoveId == null) return;
                }
    
                const xDiff = event.clientX - self.dragStart.x;
                const yDiff = event.clientY - self.dragStart.y;
                const angle = Math.atan2(yDiff, xDiff);
                const distance = Math.min(maxDistance, Math.hypot(xDiff, yDiff));
                const xPosition = distance * Math.cos(angle);
                const yPosition = distance * Math.sin(angle);
    
                // move stick image to new position
                stick.style.transform = `translate3d(${xPosition}px, ${yPosition}px, 0px)`;
    
                self.threshold = self.threshold || distance >= deadzone;
                // deadzone adjustment
                const distance2 = (distance >= deadzone || self.threshold) ? maxDistance / (maxDistance - deadzone) * (distance - deadzone) : 0;
                const xPosition2 = distance2 * Math.cos(angle);
                const yPosition2 = distance2 * Math.sin(angle);
                const xPercent = parseFloat((xPosition2 / maxDistance).toFixed(4));
                const yPercent = parseFloat((yPosition2 / maxDistance).toFixed(4));
    
                // RIGHT
                if (-Math.PI/4 <= angle && angle < Math.PI/4) {
                    _HandleButton(5, 0);
                    _HandleButton(6, 0);
                    _HandleButton(7, 0);
                    _HandleButton(4, 1);
                // LEFT
                } else if (angle >= 3*Math.PI/4 || angle < -3*Math.PI/4) {
                    _HandleButton(4, 0);
                    _HandleButton(6, 0);
                    _HandleButton(7, 0);
                    _HandleButton(5, 1);
                // UP
                } else if (-3*Math.PI/4 <= angle && angle < -Math.PI/4) {
                    _HandleButton(5, 0);
                    _HandleButton(4, 0);
                    _HandleButton(7, 0);
                    _HandleButton(6, 1);
                // DOWN
                } else {
                    _HandleButton(5, 0);
                    _HandleButton(6, 0);
                    _HandleButton(4, 0);
                    _HandleButton(7, 1);
                }
    
                self.value = { x: xPercent, y: yPercent };
              }
    
            function handleUp(event)
            {
                if ( !self.active ) return;
    
                // if this is a touch event, make sure it is the right one
                if (event.changedTouches && self.touchId != event.changedTouches[0].identifier) return;
    
                // transition the joystick position back to center
                stick.style.transition = '.2s';
                stick.style.transform = `translate3d(0px, 0px, 0px)`;
    
                // reset everything
                _HandleButton(4, 0);
                _HandleButton(5, 0);
                _HandleButton(6, 0);
                _HandleButton(7, 0);
    
                self.value = { x: 0, y: 0 };
                self.touchId = null;
                self.active = false;
                self.threshold = false;
            }
    
            stick.addEventListener('mousedown', handleDown);
            stick.addEventListener('touchstart', handleDown);
            document.addEventListener('mousemove', handleMove, {passive: false});
            document.addEventListener('touchmove', handleMove, {passive: false});
            document.addEventListener('mouseup', handleUp);
            document.addEventListener('touchend', handleUp);
        }
    }
    
    
    function SetupButton(elementId, btnid) {
        let start = "mousedown", end ="mouseup";
        if ('ontouchstart' in window) {
            start = "touchstart", end = "touchend";
        }

        const btn = document.getElementById(elementId);
        if (btn) {
          btn.addEventListener(start, function(event) {
            event.preventDefault();
            console.log("button down", performance.now())
            _HandleButton(btnid, 1);
          });
          btn.addEventListener(end, function(event) {
            event.preventDefault();
            console.log("button up", performance.now())
            _HandleButton(btnid, 0);
          });
        }
    }
    
    function SetupMacro(elementId, func) {
        const btn = document.getElementById(elementId);
        if (btn) {
          btn.addEventListener("click", function(event) {
            event.preventDefault();
            func();
          });
        }
    }
    
    function SetupJoystick(elementId) {
        new JoystickController(elementId, 20, 5);
    }
    
    function SetupIDBFS() {
        try {
            FS.mkdir('db');
            FS.mount(IDBFS, {}, 'db');
            FS.syncfs(true, function (err) {
                if (err != null) {
                    console.log("IDBFS sync failed", err);
                }
            });
            console.log("mounted idbfs");
        } catch(e) {
            console.log("idbfs mount failed", e);
            return;
        }
    
        // Set the name of the hidden property and the change event for visibility
        let hidden, visibilityChange;
        if (typeof document.hidden !== "undefined") { // Opera 12.10 and Firefox 18 and later support
            hidden = "hidden";
            visibilityChange = "visibilitychange";
        } else if (typeof document.msHidden !== "undefined") {
            hidden = "msHidden";
            visibilityChange = "msvisibilitychange";
        } else if (typeof document.webkitHidden !== "undefined") {
            hidden = "webkitHidden";
            visibilityChange = "webkitvisibilitychange";
        }
        document.addEventListener(visibilityChange, function() {
            if (document[hidden]) {
                FS.syncfs(false, function (err) {
                    if (err != null) {
                        console.log("IDBFS sync failed", err);
                    }
                });
            } else {
                console.log("visible");
            }
        }, false);
    }
    
    function SetupOpenROM(elementid) {
        const rominput = document.getElementById("rominput");
        SetupMacro(elementid, function() {
          rominput.click();
        });
        rominput.addEventListener('change', function(event){
          if (rominput.files.length === 0) {
            return;
          }
          const file = rominput.files[0];
          console.log("added rom file", file);
          const reader = new FileReader();
          reader.addEventListener('load', (event) => {
            const romdata = new Uint8Array(event.target.result);
            console.log("loaded rom data", romdata.length, romdata.BYTES_PER_ELEMENT, romdata.byteLength);
            // pass filename and rom data
            const filenameptr = allocate(intArrayFromString(file.name), ALLOC_NORMAL)
            const sramptr = allocate(intArrayFromString("db/" + file.name), ALLOC_NORMAL)
            const rombuf = _malloc(romdata.length);
            HEAPU8.set(romdata, rombuf);
            _LoadRom(filenameptr, rombuf, romdata.length, sramptr);
          });
          reader.readAsArrayBuffer(file);
        });
    }

    function ExportSRAM() {
        
    }

    return {
        SetupOpenROM,
        SetupIDBFS,
        SetupJoystick,
        SetupMacro,
        SetupButton,
        ExportSRAM,
    }
}