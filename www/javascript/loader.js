function loadSongs()
{
	var xmlhttp = new Array();
	var upper = 100;
	for(var a = 0;a < 70; a++){
		if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
			xmlhttp[a]=new XMLHttpRequest();
		} else {// code for IE6, IE5
			//should really error out
		  	xmlhttp[a]=new ActiveXObject("Microsoft.XMLHTTP");
		}


		var lower = a * parseInt(upper,10);
		var url = "http://mp/music/songs/+titles/" + lower.toString() + "-" +  upper.toString();
		xmlhttp[a].open("GET",url,true);
		//Since each xmlhttp request is an array we pass the index of it to the new function
		xmlhttp[a].onreadystatechange=function(index){
			//we then must return a function that takes no parameters
			return function(){
				if (xmlhttp[index].readyState == 4 && xmlhttp[index].status == 200){
					try{
						var myObj = JSON.parse(String(xmlhttp[index].responseText), null);
					}catch (err){
						alert("error: " + err + " on request number" + index);
						return -1;
					}
					songs_div = document.getElementById("songs");
					for(var i = 0; i < parseInt(myObj.songs.length, 10); i++){
						var bgcolor = (i%2 == 0)? "#222222" : "#666666";
			
						var newcontent = document.createElement('div');
						newcontent.style.backgroundColor= bgcolor;
						newcontent.innerHTML += "Title: " + myObj.songs[i].title + "<br \>Artist: " + myObj.songs[i].Artist +"<br \>Album:  "+myObj.songs[i].Album;
						songs_div.appendChild(newcontent);
					}
				}
		}
		}(a);
		xmlhttp[a].send();
	}	
}

function loadArtists()
{
var xmlhttp;
if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
	xmlhttp=new XMLHttpRequest();
} else {// code for IE6, IE5
	//should really error out
  	xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
}

xmlhttp.onreadystatechange=function(){
	if (xmlhttp.readyState==4 && xmlhttp.status==200){
		try{
		var myObj = JSON.parse(String(xmlhttp.responseText), null);
		}catch (err){
		alert("error: " + err);
			return -1;
		}
	for(var i = 0; i < parseInt(myObj.songs.length, 10); i++){
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";
		document.getElementById("artists").innerHTML+= "Artist: " + myObj.songs[i].Artist +"</div>";
	}
	}
}
	for(var a = 0;a < 700; a++){
		var lower = a*10;
		var upper = lower + 10 -1;
		var url = "http://mp/music/artists/+name/" + lower.toString() + "-" +  upper.toString();
		xmlhttp.open("GET",url,true);
		xmlhttp.send();
	}
}

function loadAlbums()
{
var xmlhttp;
if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
	xmlhttp=new XMLHttpRequest();
} else {// code for IE6, IE5
	//should really error out
  	xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
}

xmlhttp.onreadystatechange=function(){
	if (xmlhttp.readyState==4 && xmlhttp.status==200){
		try{
		var myObj = JSON.parse(String(xmlhttp.responseText), null);
		}catch (err){
		alert("error: " + err);
			return -1;
		}
	for(var i = 0; i < parseInt(myObj.songs.length, 10); i++){
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";
		document.getElementById("albums").innerHTML+= "<div style=\"background-color:" + bgcolor + "\">Album:  "+myObj.songs[i].Album+"</div>";
	}
	}
}
	for(var a = 0;a < 10; a++){
		var lower = a*10;
		var upper = lower + 10 -1;
		var url = "http://mp/music/albums/+names/" + lower.toString() + "-" +  upper.toString();
		xmlhttp.open("GET",url,true);
		xmlhttp.send();
	}
}
function loadUI(){
	loadSongs();
	//loadArtists();
	//loadAlbums();
}
window.onload = loadUI();