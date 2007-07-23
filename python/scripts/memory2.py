from Tkinter import *
import tkFont
import tkSimpleDialog
import tkMessageBox
from pydega import *

def askhex(prompt, validate = lambda v: None):
	v = None
	while v == None:
		s = tkSimpleDialog.askstring("Enter value", prompt+"\nFor hexadecimal prefix with 0x")
		if s == None:
			break
		try:
			v = int(eval(s))
			validate(v)
		except:
			tkMessageBox.showerror("Invalid Value", "Could not parse value: %s (%s)" % (s, sys.exc_info()[1]))
			v = None
	return v

class HexView(Canvas):

	def getdata(self):
		return getattr(self.data[0], self.data[1])

	def userchange(self, index, event):
		v = askhex("Please enter new value for address %04X" % (self.offset+index))
		if v != None:
			self.realdata[index] = v
			x = self.canvasx(event.x)
			y = self.canvasy(event.y)
			self.itemconfig(self.find_closest(x, y), text=("%02X" % v))
		
	def builditems(self):
		data = self.getdata()
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
				uc = lambda addr: lambda e: self.userchange(addr, e)
				text = self.create_text((self.fwidth*(5+3*i), y), anchor=NW, text=("%02X" % data[off+i]), font=self.font)
				self.tag_bind(text, "<Button-1>", uc(off+i))
				self.hexitems.append(text)
		self.olddata = data

	def updateitems(self):
		data = self.getdata()
		for o in range(len(self.offsets)):
			off = self.offsets[o]
			for i in range(self.columns):
				if self.olddata[off+i] <> data[off+i]:
					self.itemconfig(self.hexitems[o*self.columns+i], text=("%02X" % data[off+i]))
		self.olddata = data

	def __init__(self, master, offsets, data, realdata, columns, offset=0):
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
		self.realdata = realdata
		self.columns = columns
		self.offset = offset

		self.colitems = []
		self.hexitems = []
		self.builditems()

class Trainer:

	def getdata(self):
		return getattr(self.data[0], self.data[1])

	def __init__(self, data):
		self.data = data
		ddata = self.getdata()
		self.olddata = ddata
		self.addresses = range(len(ddata))

	def reset(self):
		data = self.getdata()
		self.olddata = data
		self.addresses = range(len(data))

	def apply(self, fn):
		data = self.getdata()
		self.addresses = filter(lambda a: fn(data[a], self.olddata[a]), self.addresses)
		self.olddata = data

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

	def nev(self, v):
		self.apply(lambda x,y: x<>v)


class MemoryViewer:

	def __init__(self, master):
		# self.frame = Frame(master)
		# self.frame.pack()

		self.frame_ram = tuple(dega.ram)
		self.frame_update = False

		self.frame = master

		self.ramtrain = Trainer((self, "frame_ram"))
		dp = lambda fn: lambda: self.doprint(fn)
		dpp = lambda fn: lambda: self.doprintprompt(fn)

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

		b = Button(self.trainframe, text="= value", command=dpp(self.ramtrain.eqv))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text="<> value", command=dpp(self.ramtrain.nev))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text="Reset", command=dp(self.ramtrain.reset))
		b.pack(side=LEFT)

		b = Button(self.trainframe, text="Add Watch", command=self.addwatch)
		b.pack(side=LEFT)

		self.hv = HexView(self.frame, offsets=range(0, len(self.frame_ram), 16), data=(self, "frame_ram"), realdata=dega.ram, columns=16, offset=0xc000)
		self.hv.pack(side=LEFT, fill=BOTH, expand=1)

		self.sb = Scrollbar(self.frame, orient=VERTICAL)
		self.sb.pack(side=LEFT, fill=Y)

		self.hv['yscrollcommand'] = self.sb.set
		self.sb['command'] = self.hv.yview

		self.wc = Label(self.frame, text="8192 matches")
		self.wc.pack(side=TOP)

		self.wv = HexView(self.frame, offsets=[], data=(self, "frame_ram"), realdata=dega.ram, columns=1, offset=0xc000)
		self.wv.pack(side=LEFT, fill=Y)

		self.sb2 = Scrollbar(self.frame, orient=VERTICAL)
		self.sb2.pack(side=RIGHT, fill=Y)

		self.wv['yscrollcommand'] = self.sb2.set
		self.sb2['command'] = self.wv.yview

		self.frameskip = 3
		self.curframe = 0

		dega.postframe = self.post_frame
		self.update_controls()

	def doprint(self, fn):
		fn()
		if len(self.ramtrain.addresses) < 128:
			self.wv.offsets = self.ramtrain.addresses
		else:
			self.wv.offsets = []
		self.wc['text'] = "%d matches" % len(self.ramtrain.addresses)
		self.wv.builditems()

	def doprintprompt(self, fn):
		v = askhex("Please enter value to compare memory to")
		if v != None:
			self.doprint(lambda: fn(v))

	def ramcheck(self, v):
		if v < 0xC000 or v >= 0xDFFF:
			raise Exception, "value outside of RAM range"

	def addwatch(self):
		v = askhex("Please enter RAM address to watch", self.ramcheck)
		if v != None:
			if len(self.ramtrain.addresses) >= 128:
				self.ramtrain.addresses = []
			self.ramtrain.addresses.append(v-0xC000)
			self.doprint(lambda: None)

	def update_controls(self):
		self.frame.after(100, self.update_controls)
		if dega.exiting:
			sys.exit(0)
		if self.frame_update:
			self.frame_update = False
			self.hv.updateitems()
			self.wv.updateitems()
	
	def post_frame(self):
#		self.curframe = (self.curframe+1)%self.frameskip
#		if self.curframe == 0:
			self.frame_ram = tuple(dega.ram)
			self.frame_update = True

root = Tk()
app = MemoryViewer(root)
root.mainloop()
