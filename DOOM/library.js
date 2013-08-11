var canvas;
var ctx;
var color;

mergeInto(LibraryManager.library, {
    initGFX: function () {
        //    alert("test");
    	canvas = document.getElementById("canvas");
	    canvas.width = "1024";
	    canvas.height = "768";
	    canvas.style.width = "1024px";
	    canvas.style.height = "768px";
	    ctx = canvas.getContext("2d");
    },
	rect:function(x,y,w,h)
	{
		ctx.fillRect(x,y,w,h);
	},
	componentToHex:function (c) {
		var hex = c.toString(16);
		return hex.length == 1 ? "0" + hex : hex;
	},
	bindColor: function (r, g, b) {
		var hexr = r.toString(16);
		hexr.length == 1 ? "0" + hexr : hexr;
		var hexg = r.toString(16);
		hexg.length == 1 ? "0" + hexg : hexg;
		var hexb = r.toString(16);
		hexb.length == 1 ? "0" + hexb : hexb;
		
		ctx.fillStyle = "#" + hexr+hexg+hexb;
	},
	getWidth:function () {
		return canvas.width;
	},
    getHeight: function() {
    	return canvas.height;
    },
    flipGFX: function () { },
    putpixel:function (x,y) {
	    _rect(x, y, 1, 1);
    },
});
/*
extern void initGFX();
extern void rect(int x,int y,int w,int h,Color c);
//	extern Color getpixel(SDL_Surface *surface, int x, int y);
extern void putpixel(int x, int y, Color pixel);
extern Color makeColor(int r,int g,int b);
extern void flipGFX();
extern int getWidth();
extern int getHeight();*/