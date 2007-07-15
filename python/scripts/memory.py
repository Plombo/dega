from Tkinter import *
from pydega import *

class MemoryViewer:

	def __init__(self, master):
		frame = Frame(master)
		frame.pack()

		self.button = Button(frame, text="Frame advance", command=self.frame_advance)
		self.button.pack(side=LEFT)

	def frame_advance(self):
		for i in range(60):
			dega.frame_advance()
		self.button.configure(text=repr(dega.ram[0:64]))

dega.load_rom(sys.argv[1])

root = Tk()
app = MemoryViewer(root)
root.mainloop()
