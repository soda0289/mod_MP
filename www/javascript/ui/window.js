function floating_box(id,top,left,width,height){
	this.f_box = document.createElement("div");
	this.f_box.id = id;
	this.f_box.style.position = "absolute";
	this.f_box.style.top = top;
	this.f_box.style.left = left;
	this.f_box.style.width = width;
	this.f_box.style.height = height;
	this.f_box.style.backgroundColor = "white";
	
	
	this.show = function(){
		var body = document.getElementById("body");
		body.appendChild(this.f_box);
	}
	
	this.appendChild = function(ch){
		this.f_box.appendChild(ch);
	}
	
	this.innerHTML = function(html){
		this.f_box.innerHTML = html;
	}

}