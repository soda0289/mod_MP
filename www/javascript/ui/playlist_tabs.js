
function playlist_tabs(parent_div, music_ui_ctx){
	
	this.div = document.createElement('div');
	
	this.tab_buttons_div = document.createElement('div');
	this.tab_buttons_div.style.height = "33x";
	this.tab_buttons_div.style.position = "relative";
	
	this.tab_buttons_ul = document.createElement('ul');
	this.tab_buttons_ul.id = "tab_buttons";
	this.tab_buttons_ul.style.fontSize = "15px";
	this.tab_buttons_ul.style.listStyleType = "none";
	this.tab_buttons_ul.style.margin = "0";
	this.tab_buttons_ul.style.padding = "0";
	
	this.tab_buttons_div.appendChild(this.tab_buttons_ul);
	
	
	
	this.tabs_div = document.createElement('div');
	this.tabs_div.style.height = "266px";
	this.tabs_div.style.position = "relative";
	
	this.div.appendChild(this.tab_buttons_div);
	this.div.appendChild(this.tabs_div);
	
	function tab(playlist_if, li){
		this.button = li;
		this.playlist = playlist_if;
		
		this.show = function(){
			this.button.className += " current_tab";
			this.playlist.div.style.display = "block";
		};
		
		this.hide = function(){
			this.button.className = "";
			this.playlist.div.style.display = "none"
		};
		
	}
	//Curent tab were showing
	this.current_tab = null;
	
	
	
	
	this.tab_click = function (p_tabs, tab){
		
			return function(e){
				//Hide current
				p_tabs.current_tab.hide();
				
				//set new current
				p_tabs.current_tab = tab;
				
				//Show current
				p_tabs.current_tab.show();
			};
	};
	
	this.add_tab = function (name, playlist_if){
		//Li button
		var li = document.createElement('li');
		
		li.innerHTML = name;
		li.style.display = "inline";
		li.style.padding = "10px";
		li.style.cursor = "pointer";
		this.tab_buttons_ul.appendChild(li);
		
		
		if(this.current_tab === null){			
			li.className += " current_tab";
		}else{
			this.current_tab.hide();
		}
		this.current_tab = new tab(playlist_if, li);
		playlist_if.insert_into_div(this.tabs_div);
		this.current_tab.show();
		li.onclick = this.tab_click(this, this.current_tab);
		
	};

	parent_div.appendChild(this.div);
}