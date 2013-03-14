function artist_album_browser(parent_div, music_ui_ctx){
	this.artists = [];
	this.albums = [];
	
	this.artist_column = [{
	 						"header" : "Artist",
	 						"friendly_name" : "artist_name"
	 					}];
	
	this.artist_click = function(aritst_album_browser){

		return function(artist){
			var query = aritst_album_browser.albums_table.query;
			return function(event){
				aritst_album_browser.artists_table.select_row(artist.index);
				query.artist_id = artist.artist_id;
				aritst_album_browser.albums_table.clear();
				load_query(query);
			}
		}
		
	}(this);
	
	this.artists_table = new table(this.artist_column, this.artist_click);


	
	this.album_column = [{
	 						"header" : "Albums",
	 						"friendly_name" : "album_name"
	 					}];
	
	this.album_click = function(aritst_album_browser){
		//Change artist table with album artist
		return function(album){
			var query = aritst_album_browser.artists_table.query;
			return function(event){
				aritst_album_browser.albums_table.select_row(album.index);
				query.album_id = album.album_id;
				aritst_album_browser.artists_table.clear();
				load_query(query);
			}
		}
		
	}(this);
	
	
	this.albums_table = new table(this.album_column, this.album_click);
	

	
	this.artists_table.query  = new music_query(music_ui_ctx.domain, 750, "artists",this.artists_table.add_rows_cb);
	this.artists_table.query.sort_by = "artist_name";
	this.albums_table.query = new music_query(music_ui_ctx.domain, 750, "albums", this.albums_table.add_rows_cb);
	this.albums_table.query.sort_by = "album_name";
	
	var artist_album_div = document.createElement("div");
	artist_album_div.style.minHeight = "200px";
	
	
	
	this.artists_table.table_div.style.display = "inline-block";
	this.artists_table.table_div.style.width = "50%";
	artist_album_div.appendChild(this.artists_table.table_div);
	

	this.albums_table.table_div.style.display = "inline-block";
	this.albums_table.table_div.style.width = "50%";
	artist_album_div.appendChild(this.albums_table.table_div);
	
	
	
	this.div = artist_album_div;
	
	load_query(this.artists_table.query);
	load_query(this.albums_table.query);
	
	parent_div.appendChild(this.div);
	
	this.artists_table.win_resize();
	this.albums_table.win_resize();
}