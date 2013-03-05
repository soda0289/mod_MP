

function playlist(parent_div,music_ui_ctx){	
	//Setup song table
	var top_height = "266px";
	
	this.songs = [];
	
	this.song_columns = [
	                {
						"header" : "Title",
						"friendly_name" : "song_title"
					},{
						"header" : "Artist",
						"friendly_name" : "artist_name"
					},{
						"header" : "Albums",
						"friendly_name" : "album_name"
					}];
	
	
	this.suffle = function (){
		//Set playlist songs array to current query results of table
		this.songs = this.songs_table.query.results;
		
		this.shuffled_playlist = new Array();
		this.shuffled_playlist[0] = this.songs[0];
		for(var i = 1;i < this.songs.length; i++){
			//Random number from [0, i]
			
			var random = Math.floor(Math.random() * (i + 1));
			this.shuffled_playlist[i] = this.shuffled_playlist[random];
			this.shuffled_playlist[random] = this.songs[i];
		}
		
		this.unshuffled_playlist = this.songs;
		this.songs = this.shuffled_playlist;
	}
	
	this.song_click = function(plist){
		
		return function(song){
			return function(event){
				if(plist.player === undefined || plist.player === null){
					alert("no player set for this playlist");
				}else{
					plist.player.audio_obj.play_song(song);
				}
			}
		}
		
	}(this);
	
	this.songs_table = new table(this.song_columns, this.song_click);
	
	this.songs_table.table_div.style.position = "absolute";
	this.songs_table.table_div.style.top = top_height;
	this.songs_table.table_div.style.bottom = "0";
	this.songs_table.table_div.style.width = "100%";
	
	this.div = this.songs_table.table_div;
	
	this.query = this.songs_table.query = new music_query(music_ui_ctx.domain, 750, "songs",this.songs_table.add_rows_cb);
	
	this.songs_table.query .sort_by = "song_title";
	
	
	load_query(this.songs_table.query);
	
	parent_div.appendChild(this.div);
	
	this.songs_table.win_resize();
}