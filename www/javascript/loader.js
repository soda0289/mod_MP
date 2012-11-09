function play_song(song_id){
	alert(song_id);
}

function print_song_table(songs, queries){
	
	songs_div = document.getElementById("songs");
	var songs_table = document.getElementById("songs_table");
	if (songs_table == null){
		songs_table = document.createElement('table');
		songs_table.id = "songs_table";
		songs_table.style.width = "100%";
		songs_div.appendChild(songs_table);
	}
	
	for(var i = 0; i < parseInt(songs.length, 10); i++){
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var new_row = document.createElement('tr');
		new_row.style.backgroundColor= bgcolor;
		new_row.style.color = "orange";
		song = songs[i];
		new_row.onclick = (function (){
			var title = song.title
			return function(){
				play_song(title);
			}
			
		})(song);
		
		var new_col = new Array();
		new_col[0] = document.createElement('td');
		new_col[0].style.width = "33%";
		new_col[0].innerHTML =  songs[i].title;
		new_row.appendChild(new_col[0]);
		new_col[1] = document.createElement('td');
		new_col[1].style.width = "33%";
		new_col[1].innerHTML =  songs[i].Artist;
		new_row.appendChild(new_col[1]);
		new_col[2] = document.createElement('td');
		new_col[2].style.width = "33%";
		new_col[2].innerHTML =  songs[i].Album;
		new_row.appendChild(new_col[2]);
		
		//newcontent.innerHTML += "Title: " + myObj.songs[i].title + "  Artist: " + myObj.songs[i].Artist +"  Album:  "+myObj.songs[i].Album;
		songs_table.appendChild(new_row);
	}
}

function print_artists(artists, queries){
	var artists_div = document.getElementById("artists");
	for(var i = 0; i < parseInt(artists.length, 10); i++){
		
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var newcontent = document.createElement('div');
		newcontent.style.backgroundColor= bgcolor;
		newcontent.style.color = "orange";
		newcontent.innerHTML += artists[i].name;
		newcontent.onclick = (function (){
			var artist_id = artist.id
			return function(){
				//clear albums
				var albums_div = document.getElementById('albums');
				var songs_div = document.getElementById('songs');
				//Stop running queries
				queries.songs_query.running = 0;
				queries.albums_query.running = 0;
				albums_div.innerHTML = "";
				songs_div.innerHTML = "";
				
				//Create two new queries
				var songs_query = new music_query("mp", 1000, "songs", "+titles", artist_id, 0, print_song_table);
				var albums_query = new music_query("mp", 100, "albums", "+albums", artist_id, 0, print_albums);
				
				songs_query.queries = queries;
				albums_query.queries = queries;
				
				//Update ui
				queries.songs_query = songs_query;
				queries.albums_query = albums_query;

				
				load_query(songs_query);
				load_query(albums_query);
				
			}
			
		})(artist = artists[i], queries);
		artists_div.appendChild(newcontent);
	}
}

function print_albums(albums, queries){
	var albums_div = document.getElementById("albums");
	for(var i = 0; i < parseInt(albums.length, 10); i++){
		
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var newcontent = document.createElement('div');
		newcontent.style.backgroundColor= bgcolor;
		newcontent.style.color = "orange";
		newcontent.innerHTML += albums[i].name;
		newcontent.onclick = (function (){
			var album_id = album.id;
			return function(){
				var songs_div = document.getElementById('songs');
				//Stop running quieres
				queries.songs_query.running = 0;
				songs_div.innerHTML = "";
				var songs_query = new music_query("mp", 1000, "songs", "+titles", 0, album_id, print_song_table);
				
				//Update ui
				queries.songs_query = songs_query;
				
				load_query(songs_query);
			}
			
		})(album = albums[i], queries);
		albums_div.appendChild(newcontent);
	}
}


function music_query(hostname, num_results, type, sort_by, artist_id,album_id, print_results_function){
	this.running = 1;
	this.count = 0;
	this.num_results = num_results;
	
	this.hostname = hostname;
	this.type = type;
	this.sort_by = sort_by;
	this.artist_id = artist_id;
	this.album_id = album_id
	this.set_url = function (){
		this.upper = (this.count * this.num_results).toString();
		this.url = "http://" + this.hostname + "/music/" + this.type + "/" + this.sort_by + "/" + this.upper + "-" + this.num_results;
		//Check if artist_id or album_id is set
		
		if (this.type == "albums" && this.artist_id > 0){
			this.url += "/artist_id/" + artist_id;
		}
		if (this.type == "songs" && (this.artist_id > 0 || this.album_id > 0)){
			if (album_id > 0){
				this.url += "/album_id/" + album_id;
			}else if (artist_id > 0){
				this.url += "/artist_id/" + artist_id;
			}
		}
	}

	this.print_results = print_results_function;
}

function music_ui(songs_query, artists_query, albums_query){
	this.songs_query = songs_query;
	this.artists_query = artists_query;
	this.albums_query = albums_query;
}

function load_query(music_query){
	
	music_query.set_url();
	var upper = music_query.num_results;

	if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
		var xmlhttp=new XMLHttpRequest();
	} else {
		//should really error out
	  	return -1;
	}


	var lower = music_query.count * parseInt(upper,10);
	var url = music_query.url;
	
	xmlhttp.open("GET",url,true);
	
	//Since each xmlhttp request is an array we pass the index of it to the new function
	xmlhttp.onreadystatechange=function(){
		//we then must return a function that takes no parameters to satisfy onreadystatechange
		return function(){
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
				try{
					var json_object = JSON.parse(String(xmlhttp.responseText), null);
				}catch (err){
					alert("error: " + err + " on request number" + index);
					return -1;
				}
				//Is the query running
				//Did the server list any results
				if(music_query.running == 1){
					if (music_query.type == "songs"){
						if (parseInt(json_object.songs.length, 10) > 0){
							music_query.print_results(json_object.songs, music_query.queries);
							music_query.count++;
							load_query(music_query);
						}
					}else if(music_query.type == "artists"){
						if (parseInt(json_object.artists.length, 10) > 0){
							music_query.print_results(json_object.artists, music_query.queries);
							music_query.count++;
							load_query(music_query);
						}
					}else if(music_query.type == "albums"){
						if (parseInt(json_object.albums.length, 10) > 0){
							music_query.print_results(json_object.albums, music_query.queries);
							music_query.count++;
							load_query(music_query);
						}	
					}
				}
				
			}
		}
	}(music_query);
	xmlhttp.send();
}

function loadUI(){
	var songs_query = new music_query("mp", 1000, "songs", "+titles", 0, 0, print_song_table);
	var artists_query = new music_query("mp", 100, "artists", "+artists", 0, 0, print_artists);
	var albums_query = new music_query("mp", 100, "albums", "+albums", 0, 0, print_albums);
	
	var music_queries = new music_ui(songs_query, artists_query, albums_query);
	
	
	songs_query.queries = music_queries;
	artists_query.queries = music_queries;
	albums_query.queries = music_queries;
	
	
	load_query(songs_query);
	load_query(artists_query);
	load_query(albums_query);
}
window.onload = loadUI();