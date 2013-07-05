
function playlist_tabs(parent_div, music_ui_ctx){
	
	this.div = document.createElement('div');
	this.div.id = "playlist_tabs";
	//Resize playlist tabs
	this.div.style.position = "absolute";
	this.div.style.bottom = "50px";
	this.div.style.right = 0;
	this.div.style.left = 0;
	
	//Div for tab buttons
	this.tab_buttons_div = document.createElement('div');
	this.tab_buttons_div.style.height = "33x";
	this.tab_buttons_div.style.position = "relative";
	//UL inline for tab buttons
	this.tab_buttons_ul = document.createElement('ul');
	this.tab_buttons_ul.id = "tab_buttons";
	this.tab_buttons_ul.style.fontSize = "15px";
	this.tab_buttons_ul.style.listStyleType = "none";
	this.tab_buttons_ul.style.margin = "0";
	this.tab_buttons_ul.style.padding = "0";
	this.tab_buttons_ul.style.height = "20px";
	
	//+ button for new tab creation
	this.new_tab_button = document.createElement('li');
	this.new_tab_button.id = "new_tab_button";
	this.new_tab_button.style.display = "inline";
	this.new_tab_button.style.fontSize = "14px";
	this.new_tab_button.style.fontWeight = "bolder";
	this.new_tab_button.style.fontFamily = "helvetica";
	this.new_tab_button.style.float = "right";
	this.new_tab_button.style.paddingRight = "5px";
	this.new_tab_button.style.paddingLeft = "5px";
	this.new_tab_button.style.backgroundColor = "orange";
	this.new_tab_button.style.height = "20px";
	this.new_tab_button.style.cursor = "pointer";
	this.new_tab_button.innerHTML = "+";
	this.new_tab_button.onclick = function (event){
		music_ui_ctx.browser_if.new_playlist_from_selection();
	};
	
	this.tab_buttons_ul.appendChild(this.new_tab_button);
	
	this.tab_buttons_div.appendChild(this.tab_buttons_ul);
	
	
	//This is the main div that holds the tabs and the playlist divs
	this.tabs_div = document.createElement('div');
	this.tabs_div.style.height = "100%";
	this.tabs_div.style.position = "relative";
	
	this.div.appendChild(this.tab_buttons_div);
	this.div.appendChild(this.tabs_div);
	
	function tab(playlist_if, li){
		this.button = li;
		this.playlist = playlist_if;
		
		this.show = function(){
			this.button.className += " current_tab";
			this.playlist.div.style.display = "block";
			this.playlist.songs_table.win_resize();
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
		playlist_if.player = music_ui_ctx.player_if;
		this.current_tab.show();
		
		
		
		li.onclick = this.tab_click(this, this.current_tab);
		
	};

	parent_div.appendChild(this.div);
}