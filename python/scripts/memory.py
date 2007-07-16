from Tkinter import *
import tkFont
from pydega import *

class HexView(Canvas):

	def builditems(self):
		self.hexitems = []
		for i in range(len(self.data)):
			y = (i/self.columns)*self.fheight
			if (i % self.columns) == 0:
				self.create_text((0, y), anchor=NW, text=("%04X" % i), font=self.font)
			self.hexitems.append(self.create_text((self.fwidth*(5+3*(i%self.columns)), y), anchor=NW, text=("%02X" % self.data[i]), font=self.font))
		self.olddata = list(self.data)

	def updateitems(self):
		for i in range(len(self.data)):
			if self.olddata[i] <> self.data[i]:
				self.itemconfig(self.hexitems[i], text=("%02X" % self.data[i]))
				self.olddata[i] = self.data[i]

	def __init__(self, master, data, columns):
		self.font = tkFont.Font(family="Courier",size=12)

		self.fwidth = self.font.measure("A")
		self.fheight = self.font.metrics("linespace")

		width = self.fwidth*(4 + columns*3) # header + data columns
		height = self.fheight*((len(data) + columns-1)/columns)

		Canvas.__init__(self, master, width=width, height=self.fheight*8,
			background="white",
			scrollregion=(0, 0, width, height))

		self.data = data
		self.columns = columns

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

		self.fadv = Button(self.frame, text="Frame advance", command=self.frame_advance)
		self.fadv.pack(side=BOTTOM)

		self.brun = Button(self.frame, text="Run", command=self.start_run)
		self.brun.pack(side=BOTTOM)

		self.ramtrain = Trainer(dega.ram)
		dp = lambda fn: lambda: self.doprint(fn)

		b = Button(self.frame, text="Reset", command=dp(self.ramtrain.reset))
		b.pack(side=BOTTOM)

		b = Button(self.frame, text="=", command=dp(self.ramtrain.eq))
		b.pack(side=BOTTOM)

		b = Button(self.frame, text="<>", command=dp(self.ramtrain.ne))
		b.pack(side=BOTTOM)

		b = Button(self.frame, text="<", command=dp(self.ramtrain.lt))
		b.pack(side=BOTTOM)

		b = Button(self.frame, text="<=", command=dp(self.ramtrain.lte))
		b.pack(side=BOTTOM)

		b = Button(self.frame, text=">", command=dp(self.ramtrain.gt))
		b.pack(side=BOTTOM)

		b = Button(self.frame, text=">=", command=dp(self.ramtrain.gte))
		b.pack(side=BOTTOM)

		self.sb = Scrollbar(self.frame, orient=VERTICAL)
		self.sb.pack(side=RIGHT, fill=Y)

		self.hv = HexView(self.frame, dega.ram, 16)
		self.hv.pack(side=LEFT, fill=Y)

		self.hv['yscrollcommand'] = self.sb.set
		self.sb['command'] = self.hv.yview

		self.running = False
		self.frameskip = 3
		self.curframe = 0

	def doprint(self, fn):
		fn()
		if len(self.ramtrain.addresses) < 128:
			print map(lambda x: (x, dega.ram[x]), self.ramtrain.addresses)
		else:
			print "too many"

	def frame_advance(self):
		if self.running:
			self.frame.after(1000/60, self.frame_advance)
		dega.frame_advance()
		self.curframe = (self.curframe+1)%self.frameskip
		if self.curframe==0 or not self.running:
			self.hv.updateitems()

	def start_run(self):
		if self.running:
			self.running = False
			return None
		self.running = True
		self.frame_advance()

# dega.load_rom(sys.argv[1])

root = Tk()
app = MemoryViewer(root)
root.mainloop()
