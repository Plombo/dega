from Tkinter import *
import tkFont
from pydega import *

class HexView(Canvas):

	def builditems(self):
		width = self.fwidth*(4 + self.columns*3) # header + data columns
		height = self.fheight*len(self.offsets)
		self.configure(scrollregion=(0, 0, width, height))

		for item in self.colitems:
			self.delete(item)
		for item in self.hexitems:
			self.delete(item)
		self.colitems = []
		self.hexitems = []
		for o in range(len(self.offsets)):
			off = self.offsets[o]
			y = o*self.fheight
			self.colitems.append(self.create_text((0, y), anchor=NW, text=("%04X" % (self.offset+off)), font=self.font))
			for i in range(self.columns):
				self.hexitems.append(self.create_text((self.fwidth*(5+3*i), y), anchor=NW, text=("%02X" % self.data[off+i]), font=self.font))
		self.olddata = list(self.data)

	def updateitems(self):
		for o in range(len(self.offsets)):
			off = self.offsets[o]
			for i in range(self.columns):
				if self.olddata[off+i] <> self.data[off+i]:
					self.itemconfig(self.hexitems[o*self.columns+i], text=("%02X" % self.data[off+i]))
					self.olddata[off+i] = self.data[off+i]

	def __init__(self, master, offsets, data, columns, offset=0):
		self.font = tkFont.Font(family="Courier",size=12)

		self.fwidth = self.font.measure("A")
		self.fheight = self.font.metrics("linespace")

		width = self.fwidth*(4 + columns*3) # header + data columns
		height = self.fheight*len(offsets)

		Canvas.__init__(self, master, width=width, height=self.fheight*16,
			background="white",
			scrollregion=(0, 0, width, height))

		self.offsets = offsets
		self.data = data
		self.columns = columns
		self.offset = offset

		self.colitems = []
		self.hexitems = []
		self.builditems()

class Trainer:

	def __init__(self, data):
		self.data = data
		self.olddata = list(data)
		self.addresses = range(len(data))

	def reset(self):
		self.olddata = list(self.data)
		self.addresses = range(len(self.data))

	def apply(self, fn):
		self.addresses = filter(lambda a: fn(self.data[a], self.olddata[a]), self.addresses)
		self.olddata = list(self.data)

	def gt(self):
		self.apply(lambda x,y: x>y)

	def gte(self):
		self.apply(lambda x,y: x>=y)

	def lt(self):
		self.apply(lambda x,y: x<y)

	def lte(self):
		self.apply(lambda x,y: x<=y)

	def eq(self):
		self.apply(lambda x,y: x==y)

	def ne(self):
		self.apply(lambda x,y: x<>y)

	def eqv(self, v):
		self.apply(lambda x,y: x==v)


class MemoryViewer:

	def __init__(self, master):
		# self.frame = Frame(master)
		# self.frame.pack()
		self.frame = master

		self.btnframe = Frame(master)
		self.btnframe.pack(side=BOTTOM)

		btncmd = lambda btn: lambda: self.inputtoggle(btn)

		self.btnup = Button(self.btnframe, text="Up", command=btncmd(BTN_UP))
		self.btnup.pack(side=LEFT)

		self.btndown = Button(self.btnframe, text="Down", command=btncmd(BTN_DOWN))
		self.btndown.pack(side=LEFT)

		self.btnleft = Button(self.btnframe, text="Left", command=btncmd(BTN_LEFT))
		self.btnleft.pack(side=LEFT)

		self.btnright = Button(self.btnframe, text="Right", command=btncmd(BTN_RIGHT))
		self.btnright.pack(side=LEFT)

		self.btnone = Button(self.btnframe, text="1", command=btncmd(BTN_1))
		self.btnone.pack(side=LEFT)

		self.btntwo = Button(self.btnframe, text="2", command=btncmd(BTN_2))
		self.btntwo.pack(side=LEFT)

		self.inputupdate()

		self.emuframe = Frame(master)
		self.emuframe.pack(side=BOTTOM)

		l = Label(self.emuframe, text="Emulation:")
		l.pack(side=LEFT)

		self.fadv = Button(self.emuframe, text="Frame advance", command=self.frame_advance)
		self.fadv.pack(side=LEFT)

		self.brun = Button(self.emuframe, text="Run", command=self.start_run)
		self.brun.pack(side=LEFT)

		self.ramtrain = Trainer(dega.ram)
		dp = lambda fn: lambda: self.doprint(fn)

		self.trainframe = Frame(master)
		self.trainframe.pack(side=BOTTOM)

		l = Label(self.trainframe, text="Trainer:")
		l.pack(side=LEFT)

		b = Button(self.trainframe, text="=", command=dp(self.ramtrain.eq))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text="<>", command=dp(self.ramtrain.ne))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text="<", command=dp(self.ramtrain.lt))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text="<=", command=dp(self.ramtrain.lte))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text=">", command=dp(self.ramtrain.gt))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text=">=", command=dp(self.ramtrain.gte))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text="Reset", command=dp(self.ramtrain.reset))
		b.pack(side=LEFT)

		self.wv = HexView(self.frame, offsets=[], data=dega.ram, columns=1, offset=0xc000)
		self.wv.pack(side=RIGHT, fill=Y)

		self.sb = Scrollbar(self.frame, orient=VERTICAL)
		self.sb.pack(side=RIGHT, fill=Y)

		self.hv = HexView(self.frame, offsets=range(0, len(dega.ram), 16), data=dega.ram, columns=16, offset=0xc000)
		self.hv.pack(side=LEFT, fill=Y)

		self.hv['yscrollcommand'] = self.sb.set
		self.sb['command'] = self.hv.yview

		self.running = False
		self.frameskip = 3
		self.curframe = 0

	def inputtoggle(self, flag):
		dega.input[0] ^= flag
		self.inputupdate()

	def inputupdatebtn(self, btn, flag):
		if dega.input[0] & flag:
			btn['relief'] = SUNKEN
		else:
			btn['relief'] = RAISED

	def inputupdate(self):
		self.inputupdatebtn(self.btnup, BTN_UP)
		self.inputupdatebtn(self.btndown, BTN_DOWN)
		self.inputupdatebtn(self.btnleft, BTN_LEFT)
		self.inputupdatebtn(self.btnright, BTN_RIGHT)
		self.inputupdatebtn(self.btnone, BTN_1)
		self.inputupdatebtn(self.btntwo, BTN_2)

	def doprint(self, fn):
		fn()
		if len(self.ramtrain.addresses) < 128:
			self.wv.offsets = self.ramtrain.addresses
		else:
			self.wv.offsets = []
		self.wv.builditems()

	def frame_advance(self):
		self.running = False
		self.brun['relief'] = RAISED
		self.do_frame_advance()

	def do_frame_advance(self):
		if self.running:
			self.frame.after(1000/60, self.do_frame_advance)
		dega.frame_advance(self.running)
		self.curframe = (self.curframe+1)%self.frameskip
		if self.curframe==0 or not self.running:
			self.hv.updateitems()
			self.wv.updateitems()

	def start_run(self):
		if self.running:
			self.running = False
			self.brun['relief'] = RAISED
			return None
		self.running = True
		self.brun['relief'] = SUNKEN
		self.do_frame_advance()

if not EMBEDDED:
	dega.load_rom(sys.argv[1])

root = Tk()
app = MemoryViewer(root)
root.mainloop()
