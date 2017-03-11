
function crypt_message(id){
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function(){
		if(xmlhttp.readyState == 4 && xmlhttp.status==200){
			var m = document.getElementById("mes" + id);
			m.textContent = xmlhttp.responseText;
		}
	}
	xmlhttp.open("GET", "cryptor?m_id=" + id);
	xmlhttp.send();
}
