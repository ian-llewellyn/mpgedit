#Boa:Frame:main_frame

from wxPython.wx import *
import mpgedit

def create(parent):
    return main_frame(parent)

[wxID_MAIN_FRAME, wxID_MAIN_FRAMEFILE_BUTTON, wxID_MAIN_FRAMEFILE_COMBO, 
 wxID_MAIN_FRAMEMAIN_PANEL, wxID_MAIN_FRAMEPLAY_PANEL, 
 wxID_MAIN_FRAMEPLAY_SLIDER, wxID_MAIN_FRAMEPLAY_VALUE_MAX_LABEL, 
 wxID_MAIN_FRAMEPLAY_VALUE_MIN_LABEL, wxID_MAIN_FRAMEPLAY_VALUE_SPINNER, 
 wxID_MAIN_FRAMESTATICBOX1, wxID_MAIN_FRAMESTATUSBAR, wxID_MAIN_FRAMETOOLBAR, 
] = map(lambda _init_ctrls: wxNewId(), range(12))

class main_frame(wxFrame):
    def _init_coll_main_boxsizer_Items(self, parent):
        # generated method, don't edit

        parent.AddWindow(self.play_panel, 0, border=0, flag=0)

    def _init_coll_statusbar_Fields(self, parent):
        # generated method, don't edit
        parent.SetFieldsCount(2)

        parent.SetStatusText(i=0, text='This is a test status')
        parent.SetStatusText(i=1, text='Fields1')

        parent.SetStatusWidths([-1, -1])

    def _init_sizers(self):
        # generated method, don't edit
        self.main_boxsizer = wxBoxSizer(orient=wxVERTICAL)

        self._init_coll_main_boxsizer_Items(self.main_boxsizer)

        self.main_panel.SetSizer(self.main_boxsizer)

    def _init_ctrls(self, prnt):
        # generated method, don't edit
        wxFrame.__init__(self, id=wxID_MAIN_FRAME, name='main_frame',
              parent=prnt, pos=wxPoint(440, 247), size=wxSize(661, 551),
              style=wxDEFAULT_FRAME_STYLE, title='py_xmpgedit')
        self.SetClientSize(wxSize(653, 517))
        self.SetStatusBarPane(1)
        self.SetThemeEnabled(False)
        self.Show(True)

        self.statusbar = wxStatusBar(id=wxID_MAIN_FRAMESTATUSBAR,
              name='statusbar', parent=self, style=0)
        self.statusbar.SetPosition(wxPoint(0, 494))
        self.statusbar.SetSize(wxSize(653, 23))
        self._init_coll_statusbar_Fields(self.statusbar)
        self.SetStatusBar(self.statusbar)

        self.main_panel = wxPanel(id=wxID_MAIN_FRAMEMAIN_PANEL,
              name='main_panel', parent=self, pos=wxPoint(0, 0),
              size=wxSize(653, 494), style=wxTAB_TRAVERSAL)
        self.main_panel.SetAutoLayout(True)

        self.play_panel = wxPanel(id=wxID_MAIN_FRAMEPLAY_PANEL,
              name='play_panel', parent=self.main_panel, pos=wxPoint(0, 0),
              size=wxSize(728, 200), style=wxTAB_TRAVERSAL)

        self.staticBox1 = wxStaticBox(id=wxID_MAIN_FRAMESTATICBOX1,
              label='Play', name='staticBox1', parent=self.play_panel,
              pos=wxPoint(32, 48), size=wxSize(520, 120), style=0)

        self.file_combo = wxComboBox(choices=[], id=wxID_MAIN_FRAMEFILE_COMBO,
              name='file_combo', parent=self.play_panel, pos=wxPoint(128, 72),
              size=wxSize(360, 21), style=0, validator=wxDefaultValidator,
              value='')
        self.file_combo.SetLabel('')

        self.file_button = wxButton(id=wxID_MAIN_FRAMEFILE_BUTTON, label='...',
              name='file_button', parent=self.play_panel, pos=wxPoint(496, 72),
              size=wxSize(24, 23), style=0)
        EVT_BUTTON(self.file_button, wxID_MAIN_FRAMEFILE_BUTTON,
              self.OnFile_buttonButton)

        self.play_slider = wxSlider(id=wxID_MAIN_FRAMEPLAY_SLIDER,
              maxValue=10000, minValue=0, name='play_slider',
              parent=self.play_panel, point=wxPoint(120, 120), size=wxSize(409,
              40), style=wxSL_HORIZONTAL, validator=wxDefaultValidator,
              value=0)
        EVT_SCROLL(self.play_slider, self.OnPlay_sliderScroll)

        self.play_value_min_label = wxStaticText(id=wxID_MAIN_FRAMEPLAY_VALUE_MIN_LABEL,
              label='0.000', name='play_value_min_label',
              parent=self.play_panel, pos=wxPoint(128, 104), size=wxSize(27,
              13), style=0)

        self.play_value_max_label = wxStaticText(id=wxID_MAIN_FRAMEPLAY_VALUE_MAX_LABEL,
              label='0.000', name='play_value_max_label',
              parent=self.play_panel, pos=wxPoint(496, 104), size=wxSize(27,
              13), style=0)

        self.play_value_spinner = wxSpinCtrl(id=wxID_MAIN_FRAMEPLAY_VALUE_SPINNER,
              initial=0, max=100, min=0, name='play_value_spinner',
              parent=self.play_panel, pos=wxPoint(48, 112), size=wxSize(72, 24),
              style=wxWANTS_CHARS | wxSP_ARROW_KEYS)
        EVT_SPINCTRL(self.play_value_spinner, wxID_MAIN_FRAMEPLAY_VALUE_SPINNER,
              self.OnPlay_value_spinnerSpinctrl)
        EVT_CHAR(self.play_value_spinner, self.OnPlay_value_spinnerChar)

        self.toolBar = wxToolBar(id=wxID_MAIN_FRAMETOOLBAR, name='toolBar',
              parent=self.play_panel, pos=wxPoint(0, 0), size=wxSize(728, 27),
              style=wxTB_HORIZONTAL | wxNO_BORDER)

        self._init_sizers()

    def __init__(self, parent):
        self._init_ctrls(parent)
        self.total_time = 0.0

    def OnFile_buttonButton(self, event):
        dlg = wxFileDialog(self, "Choose a file", ".", "", "*.mp3", wxOPEN)
        try:
            if dlg.ShowModal() == wxID_OK:
                filename = dlg.GetPath()
                # Your code
                self.file_combo.SetValue(filename) 
                self.file_combo.Append(filename)
                index = mpgedit.Index(filename)
                index.index()                
                t = index.time()                
                if t[0] == -1:
                    play = mpgedit.Play(filename)
                    t = play.total_time()
                self.total_time = t
                self.play_increment = (self.total_time[0] * 1000 + self.total_time[1]) / 10000.0                                        
                self.play_value_min_label.SetLabel('0.000')
                self.play_value_max_label.SetLabel('%d.%03d' % self.total_time)
                self.play_value_spinner.SetRange(0, self.total_time[0] * 1000 + self.total_time[1]) 
                self.play_value_spinner.SetValue(0) 
        finally:
            dlg.Destroy()

    def OnPlay_sliderScroll(self, event):        
        self.play_value_spinner.SetValue(int(self.play_slider.GetValue() * self.play_increment))
        event.Skip()

    def OnPlay_value_spinnerSpinctrl(self, event):
        self.play_slider.SetValue(int(self.play_value_spinner.GetValue() * 10000.0 / (self.total_time[0] * 1000 + self.total_time[1])))
        event.Skip()

    def OnPlay_value_spinnerChar(self, event):
        if event.GetKeyCode() == WXK_RETURN:
            self.play_slider.SetValue(int(self.play_value_spinner.GetValue() * 10000.0 / (self.total_time[0] * 1000 + self.total_time[1])))
        event.Skip()






