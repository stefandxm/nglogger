# ABANDONED BECAUSE NPYSCREEN JUST DOESNT WORK.

# encoding: utf-8
import sys
import npyscreen
sys.path.insert(1, '/home/stefan/src/xprojector/build-cpp-Desktop-Release/Logging')
import nglogger
from os import listdir
from os.path import isfile, join

def main():
    print ( nglogger.version() )
    app = NPViewer()
    app.run()
    print("i quit?")


# if len(sys.argv) < 2:
# 	filename="/var/xprojector/logs/xdb-main.nglog"
# else:
# 	filename=str(sys.argv[1])

# print("getting logfile: ", filename)
# logfile = nglogger.open(filename)
# print("getting data")

# while 1:
# 	data = logfile.read()
# 	if data is not  None:
# 		print("MESSAGE:", data.payload, "CHECKSUM OK:", data.checksumok)
# 	else:
# 		break


class NPViewer(npyscreen.StandardApp):   
    path = '/var/xprojector/logs/'
    def onStart(self):        
        self.addForm("MAIN", MainForm)

class MainForm(npyscreen.FormBaseNew):

    def create(self):    
        self.add_event_hander("event_log_select", self.event_log_select)
        self.add_event_hander("event_update_main_form", self.event_update_main_form)

        self.name = "NGLOGGER - " + self.parentApp.path
        y,x = self.useable_space()
        #self.add(npyscreen.TitleFixedText, name="Wtf2")
        #self.title = self.add(npyscreen.TitleText, name = "Text:", value= "i dont know" )
        self.loglist = self.add(LogList, name="LOGS", value= 0,relx=1, max_width= x//8, rely=1, max_height=0 )
        self.logview = self.add(LogViewer, name="LOG", value= 0,relx=(x//8)+1,  rely=1, max_height=0 )
        #/self.how_exited_handers[npyscreen.wgwidget.EXITED_ESCAPE] = self.exit_application
        #npyscreen.notify_confirm(repr(self.parentApp))


        new_handlers = {
            "^Q": self.exit_application,
            155: self.exit_application,
            "^U": self.event_log_select
        }
        self.add_handlers(new_handlers)

        self.loglist.update_logs()

    def event_update_main_form(self, event):
        self.display()
        self.loglist.display()
        self.logview.display()

    def event_log_select(self, event):
        self.logview.update_log()                
        # current_user = self.chatBoxObj.value
        # client.dialogs[current_user].unread_count = 0

        # self.chatBoxObj.update_chat()
        # self.messageBoxObj.update_messages(current_user)

        # client.read_all_messages(current_user)

    def exit_application(self, event):
        self.parentApp.setNextForm(None)
        self.editing = False

    def while_waiting(self):
        pass

    def when_value_edited(self):        
        self.parent.parentApp.queue_event(npyscreen.Event("event_log_select"))


class LogList(npyscreen.BoxTitle):
    
    def when_value_edited(self):        
        self.parent.parentApp.queue_event(npyscreen.Event("event_log_select"))
            
    def update_logs(self):
        path = self.parent.parentApp.path
        data = [f for f in listdir(path) if isfile(join(path, f))]

        # data = []
        # data.append("hej")
        # data.append("idiot")
        self.values =data

        self.parent.parentApp.queue_event(npyscreen.Event("event_update_main_form"))


class LogViewer(npyscreen.BoxTitle):
    def when_value_edited(self):
        pass

    def update_log(self):
        data = []
        data.append("vafan")
        data.append("mongo")
        self.values =data

        self.parent.parentApp.queue_event(npyscreen.Event("event_update_main_form"))

if __name__ == '__main__':
    main()
