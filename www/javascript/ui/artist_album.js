function artist_album_browser(parent_div, music_ui_ctx){
	this.artists = [];
	this.albums = [];
	
	this.artist_column = [{"header" : "Artist","friendly_name" : "artist_name"}];
	
	this.new_playlist_from_selection = function(){
		var parameters = new query_parameters("songs");
		parameters.artist_id = this.albums_table.query.parameters.artist_id.concat(this.artists_table.query.parameters.artist_id);
		parameters.album_id = this.artists_table.query.parameters.album_id.concat(this.albums_table.query.parameters.album_id);
		parameters.sort_by = "song_title";
		parameters.num_results = 1000;
		
		var new_playlist = new playlist(music_ui_ctx.domain, parameters);
		music_ui_ctx.playlist_tabs_if.add_tab("new 1", new_playlist);
		
		this.albums_table.clear();
		this.artists_table.clear();
		this.albums_table.query.parameters.artist_id = [];
		this.albums_table.query.load();
		this.artists_table.query.parameters.album_id = [];
		this.artists_table.query.load();
		
	};
	
	this.artist_click = function(aab){

		return function(artist){
			var query = aab.albums_table.query;
			return function(event){
				var right = 0;
				//Check if right click
				if ("which" in event){
					// Gecko (Firefox), WebKit (Safari/Chrome) & Opera
					right = (event.which === 2); 
				}else if ("button" in event){  // IE, Opera 
					right = (event.button === 1); 
				}
				
				if(right){
					if (event.stopPropagation){
			            event.stopPropagation();
					}
			        event.cancelBubble = true;
					aab.new_playlist_from_selection();
				}else{
					aab.artists_table.select_row(artist.index);
					query.parameters.artist_id.push(artist.artist_id);
					aab.albums_table.clear();
					query.load();
				
				}
				return false;
			};
		};
		
	}(this);
	
	//Double click adds to current playlist except All
	this.artists_dblclick = function(){
		
	};
	
	this.artists_table = new table(this.artist_column, this.artist_click);


	
	this.album_column = [{"header" : "Albums","friendly_name" : "album_name"}];
	
	this.album_click = function(aritst_album_browser){
		//Change artist table with album artist
		return function(album){
			var query = aritst_album_browser.artists_table.query;
			return function(event){
				aritst_album_browser.albums_table.select_row(album.index);
				query.parameters.album_id.push(album.album_id);
				aritst_album_browser.artists_table.clear();
				query.load();
			};
		};
		
	}(this);
	
	
	this.albums_table = new table(this.album_column, this.album_click);
	
	this.artist_parameters = new query_parameters("artists");
	this.artist_parameters.num_results = "750";
	
	this.artists_table.query = new music_query(music_ui_ctx.domain, this.artist_parameters,this.artists_table.add_rows_cb);
	this.artists_table.query.sort_by = "artist_name";
	
	this.album_parameters = new query_parameters("albums");
	this.album_parameters.num_results = "750";
	
	this.albums_table.query = new music_query(music_ui_ctx.domain, this.album_parameters, this.albums_table.add_rows_cb);
	this.albums_table.query.sort_by = "album_name";
	
	var artist_album_div = document.createElement("div");
	artist_album_div.style.minHeight = "200px";
	
	
	
	this.artists_table.table_div.style.display = "inline-block";
	this.artists_table.table_div.style.width = "50%";
	this.artists_table.table_div.style.height = "200px";
	artist_album_div.appendChild(this.artists_table.table_div);
	

	this.albums_table.table_div.style.display = "inline-block";
	this.albums_table.table_div.style.width = "50%";
	this.albums_table.table_div.style.height = "200px";
	artist_album_div.appendChild(this.albums_table.table_div);
	
	
	
	this.div = artist_album_div;
	
	this.artists_table.query.load();
	this.albums_table.query.load();
	
	parent_div.appendChild(this.div);
	
	this.artists_table.win_resize();
	this.albums_table.win_resize();
}