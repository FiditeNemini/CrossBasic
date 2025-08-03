

Function PIT(g as XGraphics) as XGraphics
	g.clear()
	g.drawline(0,0,54,50)
	return g
End Function


var x as new XGraphics(64,64,32)
print(str(x.handle))
print(str(x.width) + ", " + str(x.height))


var pp as New XPicture(64,64,32)
print(str(pp.width) + ", " + str(pp.height))
pp.save("C:\Users\mcomb\Documents\GitHub\CrossBasic\Releases\release-x64-windows\mypic.png")


var ff as New XPicture
ff.load("ico.png")
print(str(pp.width) + ", " + str(pp.height))
pp.save("C:\Users\mcomb\Documents\GitHub\CrossBasic\Releases\release-x64-windows\mypic1.png")

var c as XGraphics = PIT(pp.Graphics)
print(str(c.width) + ", " + str(c.height))
c.savetofile("C:\Users\mcomb\Documents\GitHub\CrossBasic\Releases\release-x64-windows\mypic2.png")

