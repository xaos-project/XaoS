resource('MIMS', 1, "BEOS:APP_SIG") {
	import("XaoSResources", 'MIMS', 1)
}

resource('MSGG', 1, "BEOS:FILE_TYPES") {
	import("XaoSResources", 'MSGG', 1)
}

resource('APPV', 1, "BEOS:APP_VERSION") {
	import("XaoSResources", 'APPV', 1)
}

resource('APPF', 1, "BEOS:APP_FLAGS") {
	import("XaoSResources", 'APPF', 1)
}

resource('ICON', 101, "BEOS:L:STD_ICON") {
	import("XaoSResources", 'ICON', 101)
}
resource('ICON', 0, "BEOS:L:image/x-xaos-position") {
	import("XaoSResources", 'ICON', 0)
}
resource('ICON', 1, "BEOS:L:video/x-xaos-animation") {
	import("XaoSResources", 'ICON', 1)
}

resource('MICN', 101, "BEOS:M:STD_ICON") {
	import("XaoSResources", 'MICN', 101)
}
resource('MICN', 0, "BEOS:M:image/x-xaos-position") {
	import("XaoSResources", 'MICN', 0)
}
resource('MICN', 1, "BEOS:M:video/x-xaos-animation") {
	import("XaoSResources", 'MICN', 1)
}

resource('XaSp', 1, "Splash screen") {
	read("SplashScreen")
}
