#!/bin/sh

cat << HEADER
<script type="text/javascript">
<!--
function querySt(ji) {
	hu = window.location.search.substring(1);
	gy = hu.split("&");
	for (i=0;i<gy.length;i++) {
		ft = gy[i].split("=");
		if (ft[0] == ji) {
			return ft[1];
		}
	}
}
var ref = querySt("ref");
var reftarget = new Object();
HEADER

grep '^\.\. _[[:alnum:]]*:' *.txt | sed 's/\(^.*\)\.txt:\.\. _\(.*\):/reftarget\["\2"\] = "\1.html#\2";/' | sort | uniq

cat << FOOTER
window.location = reftarget[ref];
-->
</script>
FOOTER
