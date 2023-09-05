#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const char *DEFAULT_DISPLAY = ":0";
const bool DEFAULT_PRINT_UP = true;

int main(int argc, char *argv[]) {
  const char *xDisplayName = DEFAULT_DISPLAY;
  bool printKeyUps = DEFAULT_PRINT_UP;

  Display *disp = XOpenDisplay(DEFAULT_DISPLAY);
  if (NULL == disp) {
    fprintf(stderr, "cannot open xdisplay '%s'\n", DEFAULT_DISPLAY);
    exit(1);
  }

  int xiOpcode;
  {
    int queryEvent, queryError;
    if (!XQueryExtension(disp, "XInputExtension", &xiOpcode, &queryEvent,
                         &queryError)) {
      fprintf(stderr, "X input extension not available\n");
      exit(2);
    }
  }
  {
    int major = 2, minor = 0;
    int queryResult = XIQueryVersion(disp, &major, &minor);
    if (queryResult == BadRequest) {
      fprintf(stderr, "need xi 2.0 support (got %d.%d)\n", major, minor);
      exit(3);
    } else if (queryResult != Success) {
      fprintf(stderr, "xiqueryversion failed!\n");
      exit(4);
    }
  }
  {
    Window root = DefaultRootWindow(disp);
    XIEventMask m;
    m.deviceid = XIAllMasterDevices;
    m.mask_len = XIMaskLen(XI_LASTEVENT);
    m.mask = calloc(m.mask_len, sizeof(char));

    XISetMask(m.mask, XI_RawKeyPress);
    if (printKeyUps)
      XISetMask(m.mask, XI_RawKeyRelease);
    XISelectEvents(disp, root, &m, 1);
    XSync(disp, false);
    free(m.mask);
  }
  int xkbOpcode, xkbEventCode;
  {
    int queryError, majorVersion, minorVersion;
    if (!XkbQueryExtension(disp, &xkbOpcode, &xkbEventCode, &queryError,
                           &majorVersion, &minorVersion)) {
      fprintf(stderr, "xkb ext not available");
      exit(2);
    }
  }
  XkbSelectEventDetails(disp, XkbUseCoreKbd, XkbStateNotify, XkbGroupStateMask,
                        XkbGroupStateMask);
  int group;
  {
    XkbStateRec state;
    XkbGetState(disp, XkbUseCoreKbd, &state);
    group = state.group;
  }

  while (1) {
    XEvent event;
    XGenericEventCookie *cookie = (XGenericEventCookie *)&event.xcookie;
    XNextEvent(disp, &event);

    if (XGetEventData(disp, cookie)) {
      if (cookie->type == GenericEvent && cookie->extension == xiOpcode) {
        if (cookie->evtype == XI_RawKeyRelease ||
            cookie->evtype == XI_RawKeyPress) {
          XIRawEvent *ev = cookie->data;
          KeySym s = XkbKeycodeToKeysym(disp, ev->detail, group, 0);

          if (NoSymbol == s) {
            if (group == 0)
              continue;
            else {
              s = XkbKeycodeToKeysym(disp, ev->detail, 0, 0);
              if (NoSymbol == s)
                continue;
            }
          }
          char *str = XKeysymToString(s);
          if (NULL == str)
            continue;

          if (printKeyUps)
            printf("%s", cookie->evtype == XI_RawKeyPress ? "+" : "-");
          printf("%s\n", str);
          fflush(stdout);
        }
      }
      XFreeEventData(disp, cookie);
    } else {
      if (event.type == xkbEventCode) {
        XkbEvent *xkbevent = (XkbEvent *)&event;
        if (xkbevent->any.xkb_type == XkbStateNotify) {
          group = xkbevent->state.group;
        }
      }
    }
  }
}
