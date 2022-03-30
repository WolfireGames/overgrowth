function get(id) {
  return document.getElementById(id);
}

function hide(id) {
  var o = get(id);
  if( o != null ) o.style.display = "none";
}

function show(id) {
  hide('intro'); 
  hide('example');
  hide('howto');
  hide('copyright');
  hide('download');
  hide('links');
  hide('author');
  hide('faq');

  var o = get(id);
  if( o == null ) return;

  o.style.display = "block";
}
