// Code provided by Alex Andreotti based on the information found here:
// http://chris-barr.com/2010/05/scrolling_a_overflowauto_element_on_a_touch_screen_device/

function isTouchDevice(){
	try{
		document.createEvent("TouchEvent");
		return true;
	}catch(e){
		return false;
	}
}

function touchScroll(el){
	if(isTouchDevice()){
		var scrollStartPos=0;
		var lastPos=0;
		var delta=0;
		var capture=false;

		el.addEventListener("touchstart", function(event) {
			scrollStartPos=this.scrollTop+event.touches[0].pageY;
			lastPos = event.touches[0].pageY;
			if (capture) {
				event.preventDefault();
				capture = false;
			}
		},false);

		el.addEventListener("touchmove", function(event) {
			var deltaY = scrollStartPos-event.touches[0].pageY;
			delta = event.touches[0].pageY - lastPos;
			//document.title = deltaY+':'+lastPos+':'+delta;
			this.velocity = delta;
			lastPos = event.touches[0].pageY;
			if (delta == 0) {
				capture = false;
			} else {
				capture = !(delta <= 0 && this.scrollTop+this.clientHeight==this.scrollHeight) && !(delta >= 0 && this.scrollTop == 0);
			}
			if (capture) {
				this.scrollTop = deltaY;
				event.preventDefault();
			}
		},false);
	}
}

// Add the touchScroll event handler to the nav-tree and main content divs
$(document).ready(function() {
	$('#doc-content, #nav-tree').each(function(){ touchScroll(this); });
});

