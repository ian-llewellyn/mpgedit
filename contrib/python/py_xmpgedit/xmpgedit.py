#!/usr/bin/env python
#Boa:App:BoaApp

from wxPython.wx import *

import main_frame

modules ={'main_frame': [1, 'Main frame of Application', 'main_frame.py']}

class BoaApp(wxApp):
    def OnInit(self):
        wxInitAllImageHandlers()
        self.main = main_frame.create(None)
        # needed when running from Boa under Windows 9X
        self.SetTopWindow(self.main)
        self.main.Show();self.main.Hide();self.main.Show()
        return True

def main():
    application = BoaApp(0)
    application.MainLoop()

if __name__ == '__main__':
    main()
