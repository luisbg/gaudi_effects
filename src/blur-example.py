#!/usr/bin/python
import gobject; gobject.threads_init()
import pygst; pygst.require("0.10")
import gst

pipestr = "videotestsrc ! ffmpegcolorspace ! queue ! chromium name=vf ! ffmpegcolorspace ! timeoverlay ! xvimagesink"
p = gst.parse_launch (pipestr)

m = p.get_by_name ("vf")
m.set_property ("edge-a", 50)

control = gst.Controller(m, "edge-a")
control.set_interpolation_mode("edge-a", gst.INTERPOLATE_LINEAR)
control.set("edge-a", 0 * gst.SECOND, 50)
control.set("edge-a", 5 * gst.SECOND, 100)
control.set("edge-a", 25 * gst.SECOND, 200)

p.set_state (gst.STATE_PLAYING)

gobject.MainLoop().run()
