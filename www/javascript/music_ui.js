

//Music UI context object
function music_ui(songs_query, artists_query, albums_query){
	this.songs_query = songs_query;
	this.artists_query = artists_query;
	this.albums_query = albums_query;
	
	//Create array that links to each query
	//this is only used in the for statement
	this.queries = new Array(this.songs_query, this.artists_query, this.albums_query);
	
	this.songs = new Array();
	this.albums = new Array();
	this.artists = new Array();
	
	try {
		this.audio_ctx = new webkitAudioContext();
	} catch(e) {
		alert('Web Audio API is not supported in this browser');
	}
	this.source = null;
	this.source_loaded = 0;
	this.source_next = null;
}


function loadUI(){
	
	//Create queries for Songs, Artists, and Albums
	var songs_query = new music_query("mp", 1000, "songs", "+titles", 0, 0, print_song_table);
	var artists_query = new music_query("mp", 100, "artists", "+artists", 0, 0, print_artists);
	var albums_query = new music_query("mp", 100, "albums", "+albums", 0, 0, print_albums);
	
	//Load music ui
	var music_ui_ctx = new music_ui(songs_query, artists_query, albums_query);
	
	load_queries(music_ui_ctx);
}
window.onload = loadUI();