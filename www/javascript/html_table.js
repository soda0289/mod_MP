function print_song_table(music_ui_ctx){
	
	var songs = music_ui_ctx.songs;
	
	for(var i = music_ui_ctx.songs_table.rows.length; i < music_ui_ctx.songs.length; i = music_ui_ctx.songs_table.rows.length){
		var bgcolor = (i%2 == 0)? "#444444" : "#666666";

		var new_row = document.createElement('tr');
		
		new_row.id = "song_" + songs[i].song_id;
		new_row.style.backgroundColor= bgcolor;
		new_row.style.color = "orange";
		
		new_row.borderWidth = "0px";
		new_row.borderColor = "black";
		new_row.borderStyle = "Solid";
	
		
		new_row.onclick = function (songs, index, music_ui_ctx){
			var song = songs[index];
			
			return function(){
				music_ui_ctx.playing_index = index;
				play_song(song, music_ui_ctx);
			}
			
		}(songs, i, music_ui_ctx);
		
		var new_col = new Array();
		new_col[0] = document.createElement('td');
		new_col[0].style.width = "33%";
		new_col[0].innerHTML =  songs[i].song_title;
		new_row.appendChild(new_col[0]);
		new_col[1] = document.createElement('td');
		new_col[1].style.width = "33%";
		new_col[1].innerHTML =  songs[i].artist_name;
		new_row.appendChild(new_col[1]);
		new_col[2] = document.createElement('td');
		new_col[2].style.width = "33%";
		new_col[2].innerHTML =  songs[i].album_name;
		new_row.appendChild(new_col[2]);
		
		//newcontent.innerHTML += "Title: " + myObj.songs[i].title + "  Artist: " + myObj.songs[i].Artist +"  Album:  "+myObj.songs[i].Album;
		music_ui_ctx.songs_table.appendChild(new_row);
	}
}

function print_artists(music_ui_ctx){
	var artists_div = document.getElementById("artists");
	for(var i = artists_div.getElementsByTagName('div').length; i < music_ui_ctx.artists.length; i = artists_div.getElementsByTagName('div').length){
		
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var newcontent = document.createElement('div');
		newcontent.style.backgroundColor= bgcolor;
		newcontent.style.color = "orange";
		newcontent.innerHTML += music_ui_ctx.artists[i].artist_name;
		newcontent.onclick = (function (artist, music_ui_ctx){
			var artist_id = artist.artist_id
			return function(){
				//clear albums
				var albums_div = document.getElementById('albums');
				var songs_div = document.getElementById('songs');
				//Stop running queries
				music_ui_ctx.songs_query.running = 0;
				music_ui_ctx.albums_query.running = 0;
				albums_div.innerHTML = "";
				songs_div.innerHTML = "";
				
				//Clear old query
				music_ui_ctx.songs_query.count = 0;
				music_ui_ctx.songs = new Array();
				
				music_ui_ctx.create_songs_table();
				
				music_ui_ctx.albums_query.count = 0;
				music_ui_ctx.albums = new Array();
				
				//Create two new queries
				music_ui_ctx.songs_query.running = 1;
				music_ui_ctx.songs_query.artist_id = artist_id;
				
				load_query(music_ui_ctx.songs_query, music_ui_ctx);
				
				music_ui_ctx.albums_query.running = 1;
				music_ui_ctx.albums_query.artist_id = artist_id;
				load_query(music_ui_ctx.albums_query, music_ui_ctx);
				
			}
			
		})(artist = music_ui_ctx.artists[i], music_ui_ctx);
		artists_div.appendChild(newcontent);
	}
}

function print_albums(music_ui_ctx){
	var albums_div = document.getElementById("albums");
	for(var i = albums_div.getElementsByTagName('div').length; i < music_ui_ctx.albums.length; i = albums_div.getElementsByTagName('div').length){
		
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var newcontent = document.createElement('div');
		newcontent.style.backgroundColor= bgcolor;
		newcontent.style.color = "orange";
		newcontent.innerHTML += music_ui_ctx.albums[i].album_name;
		newcontent.onclick = (function (album, music_ui_ctx){
			var album_id = album.album_id;
			var artist_id = album.artist_id;
			return function(){
				var songs_div = document.getElementById('songs');
				//Stop running quieres
				music_ui_ctx.songs_query.running = 0;
				songs_div.innerHTML = "";
				music_ui_ctx.songs = new Array();
				music_ui_ctx.songs_query.count = 0;
				
				music_ui_ctx.songs_query.running = 1;
				music_ui_ctx.songs_query.artist_id = artist_id;
				music_ui_ctx.songs_query.album_id = album_id;
				load_query(music_ui_ctx.songs_query, music_ui_ctx);
			}
			
		})(album = music_ui_ctx.albums[i], music_ui_ctx);
		albums_div.appendChild(newcontent);
	}
}