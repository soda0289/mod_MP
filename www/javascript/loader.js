function play_song(song_id){
	alert(song_id);
}

function print_song_table(songs){
	
	songs_div = document.getElementById("songs");
	var songs_table = document.getElementById("songs_table");
	if (songs_table == null){
		songs_table = document.createElement('table');
		songs_table.id = "songs_table";
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

function print_artists(artists){
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
				albums_div.innerHTML = "";
				songs_div.innerHTML = "";
				loadAlbums(0, artist_id);
				loadSongs(0, artist_id, 0)
			}
			
		})(artist = artists[i]);
		artists_div.appendChild(newcontent);
	}
}

function print_albums(albums){
	var albums_div = document.getElementById("albums");
	for(var i = 0; i < parseInt(albums.length, 10); i++){
		
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var newcontent = document.createElement('div');
		newcontent.style.backgroundColor= bgcolor;
		newcontent.style.color = "orange";
		newcontent.innerHTML += albums[i].name;
		albums_div.appendChild(newcontent);
	}
}

function loadSongs(a, artist_id, album_id){
	//this code is so stupid
	var upper = 100;

	if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
		var xmlhttp=new XMLHttpRequest();
	} else {
		//should really error out
	  	return -1;
	}


	var lower = a * parseInt(upper,10);
	if (parseInt(artist_id) > 0){
		var url = "http://mp/music/songs/+titles/" + lower.toString() + "-" +  upper.toString() + "/artist_id/" + artist_id;
	}else if (parseInt(album_id) > 0){
		var url = "http://mp/music/songs/+titles/" + lower.toString() + "-" +  upper.toString() + "/album_id/" + album_id;
	}else {
		var url = "http://mp/music/songs/+titles/" + lower.toString() + "-" +  upper.toString();
	}
	
	xmlhttp.open("GET",url,true);
	//Since each xmlhttp request is an array we pass the index of it to the new function
	xmlhttp.onreadystatechange=function(index, art_id, al_id){
		//we then must return a function that takes no parameters to satisfy onreadystatechange
		return function(){
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
				try{
					var json_object = JSON.parse(String(xmlhttp.responseText), null);
				}catch (err){
					alert("error: " + err + " on request number" + index);
					return -1;
				}
				//Did the server list any results
				if (parseInt(json_object.songs.length, 10) > 0){
						print_song_table(json_object.songs);
						loadSongs(++index, art_id, al_id);
				}
			}
		}
	}(a, artist_id, album_id);
	xmlhttp.send();
}

function loadArtists(a)
{
	//this code is so stupid
	var upper = 100;

	if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
		var xmlhttp=new XMLHttpRequest();
	} else {// code for IE6, IE5
		//should really error out
	  	return -1;
	}


	var lower = a * parseInt(upper,10);
	var url = "http://mp/music/artists/+titles/" + lower.toString() + "-" +  upper.toString();
	xmlhttp.open("GET",url,true);
	//Since each xmlhttp request is an array we pass the index of it to the new function
	xmlhttp.onreadystatechange=function(index){
		//we then must return a function that takes no parameters to satisfy onreadystatechange
		return function(){
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
				try{
					var json_object = JSON.parse(String(xmlhttp.responseText), null);
				}catch (err){
					alert("error: " + err + " on request number" + index);
					return -1;
				}
				if (parseInt(json_object.artists.length, 10) > 0){
					print_artists(json_object.artists);
					loadArtists(++index);
				}
			}
		}
	}(a);
	xmlhttp.send();
}

function loadAlbums(a, id)
{
	//this code is so stupid
	var upper = 100;

	if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
		var xmlhttp=new XMLHttpRequest();
	} else {// code for IE6, IE5
		//should really error out
	  	return -1;
	}


	var lower = a * parseInt(upper,10);
	if (parseInt(id) == 0 || id == undefined){
		var url = "http://mp/music/albums/+titles/" + lower.toString() + "-" +  upper.toString();
	} else {
		var url = "http://mp/music/albums/+titles/" + lower.toString() + "-" +  upper.toString() + "/artist_id/" + id;
	}
	xmlhttp.open("GET",url,true);
	//Since each xmlhttp request is an array we pass the index of it to the new function
	xmlhttp.onreadystatechange=function(index,aid){
		//we then must return a function that takes no parameters to satisfy onreadystatechange
		return function(){
			if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
				try{
					var json_object = JSON.parse(String(xmlhttp.responseText), null);
				}catch (err){
					alert("error: " + err + " on request number" + index);
					return -1;
				}
				
				if (parseInt(json_object.albums.length, 10) > 0){
					print_albums(json_object.albums);
					loadAlbums(++index, aid);
				}
			}
		}
	}(a, id);
	xmlhttp.send();
}
function loadUI(){
	loadSongs(0, 0, 0);
	loadArtists(0);
	loadAlbums(0, 0);
}
window.onload = loadUI();