'use strict';

var url = ''

function pand2(i) {
	var o = i
	if (i < 10) {o = "0" + o}
	return o;
}
function toTimeStr(t) {
	var ss = pand2(t % 60)
	t = Math.floor(t / 60)
	var mm = pand2(t % 60)
	t = Math.floor(t / 60)
	var hh = pand2(t % 60)
	return hh + ':' + mm + ':' + ss
}

function round2(i) {
	var n = Math.round(parseFloat(i) * 100)
	var sub = n % 100
	var out = sub
	if (sub < 10) {out = "0" + out}
	return Math.round(n / 100.0) + '.' + out
}

function msg(hdr, str) {
	var e = $('#msg')
	e.find('div.header > h2').text(hdr)
	e.find('.content > p').text(str)
	e.css('display', 'block')
}

function setdata(u, parms, cb, errcb) {
	var key = sha256.hex($('#key').val())
	// parms = 'a=123&i=0'...
	// u = '/sch/def?'
	$.ajax({
	url: url + '/token',
	type: 'GET',
	crossDomain: true,
	error: console.log,
	success: function(hex){
		console.log('/token', hex)
		var iv = aesjs.utils.hex.toBytes(hex);
		var parmsBytes = sha256.array(parms);
		var keyBytes = aesjs.utils.hex.toBytes(key);

		var aesCbc = new aesjs.ModeOfOperation.cbc(keyBytes, iv);
		var encryptedBytes = aesCbc.encrypt(parmsBytes);

		parms += '&k=' + aesjs.utils.hex.fromBytes(encryptedBytes);

		$.ajax({
		url: url + u + parms,
		type: 'GET',
		crossDomain: true,
		error: function(e){
			console.log('send err', e)
			if(errcb) errcb(e)
		},
		success: function(obj){
			console.log('send ret', obj)
			if(typeof cb === "function") cb(obj)
		}})
	}})
}

function syscall(code, cb) {
	var parm = 'c=' + code
	setdata('/sys?', parm, function(obj){
		console.log('/sys', obj)
		if(typeof cb === "function") cb(obj)
	}, function(e){
		console.log('/sys err', e)
		msg('System Error', e.status + ' - ' + e.statusText)
	});
}

var logs = {
	temp: new TimeSeries(),
	hum: new TimeSeries(),
	press: new TimeSeries(),
}

var stEle = mkStatus()
function mkStatus() {
	var e = $('#status')
	var out = {}
	out.e = e

	var p = ["temp", "hum", "press", "pin", "on", "off", "heap"]
	p.forEach(function(v,k){
		out[v] = e.find('div:nth-child(' + (k+1)+ ') > div:nth-child(2)')
	})

	return out
}
function getStatus(cb) {
	$.ajax({
	url: url + '/status',
	type: 'GET',
	crossDomain: true,
	success: function(obj){
		//console.log('/status', obj)
		updateStatusEle(stEle, obj)
		if(typeof cb === "function") cb(obj)
	}})
}
function updateStatusEle(e, o) {
	e.pin.text( (o.pin) ? 'ON' : 'OFF' )
	e.heap.text(o.heap)
	if(o.md) {
		e.on.text( o.md[0] )
		e.off.text( o.md[1] )
	}
	if(o.sen) {
		var temp = o.sen[0] / 32.0
		var hum = o.sen[1] / 40.0
		var press = (o.sen[2] / 200.0) + 1013.0
		e.temp.text( round2(temp) )
		e.hum.text( round2(hum) )
		e.press.text( round2(press) )

		var now = new Date().getTime()
		logs.temp.append(now, temp)
		logs.hum.append(now, hum)
		logs.press.append(now, press)
	}
}

function getSetting(cb) {
	$.ajax({
	url: url + '/setting',
	type: 'GET',
	crossDomain: true,
	success: function(o){
		console.log('/setting', o)

		$('#wifi-mode').val(o.mode)
		$('#ap-ssid').val(o.ap)
		$('#ap-hide').prop('checked', o.hide)
		$('#ap-chan').val(o.ch)
		$('#sta-ssid').val(o.sta)

		$('#pwr-sleep').val(o.pwr)

		if(typeof cb === "function") cb(o)
	}})
}

function setSetting(parms, cb) {
	var key = sha256.hex($('#key').val())
	$.ajax({
	url: url + '/token',
	type: 'GET',
	crossDomain: true,
	error: console.log,
	success: function(hex){
		console.log('/token', hex)
		var iv = aesjs.utils.hex.toBytes(hex);
		var parmsBytes = aesjs.utils.utf8.toBytes(parms);
		var keyBytes = aesjs.utils.hex.toBytes(key);

		var aesCtr = new aesjs.ModeOfOperation.ctr(keyBytes, new aesjs.Counter(iv));
		var encryptedBytes = aesCtr.encrypt(parmsBytes);

		parms = aesjs.utils.hex.fromBytes(encryptedBytes);

		$.ajax({
		url: url + '/setting',
		data: 'c=' + parms,
		type: 'POST',
		crossDomain: true,
		error: function(e){
			console.log('send err', e)
		},
		success: function(obj){
			console.log('send ret', obj)
			if(typeof cb === "function") cb(obj)
		}})
	}})
}


function getSchedule(cb) {
	$.ajax({
	url: url + '/sch/ls',
	type: 'GET',
	crossDomain: true,
	error: console.log,
	success: function(obj){
		console.log('/sch/ls', obj)
		updateSchEle(obj)
		if(typeof cb === "function") cb(obj)
	}})
}
function updateSchEle(o) {
	var def = $('#sch-def')
	var cus = $('#sch-cus')

	// update default output rule
	for(var i=0; i<7; i++){
		var e = def.find('div > div:nth-child(' + (i+2) + ')')
		$(e[0]).attr('on', o.def[i][0]).attr('of', o.def[i][1])
		$(e[1]).text(o.def[i][0])
		$(e[2]).text(o.def[i][1])
	}

	// update custom output rule
	var tmpl = $(cus.find('> div:nth-child(1)')).clone()
	cus.html('')
	tmpl.on('click', popCus).appendTo(cus)
	var wmap = [7,1,2,3,4,5,6]
	for(var i=0; i<o.sch.length; i++){
		var sch = o.sch[i]
		var e = tmpl.clone()
		e.attr('mid', i)
		e.attr('w', sch[0])
		e.attr('as', sch[1])
		e.attr('bs', sch[2])
		e.attr('on', sch[3])
		e.attr('of', sch[4])
		e.find('> div:nth-child(1)').text('')
		e.find('> div:nth-child(2)').text('w' + wmap[sch[0]]) // week
		e.find('> div:nth-child(3)').text(toTimeStr(sch[1])) // start
		e.find('> div:nth-child(4)').text(toTimeStr(sch[2])) // end
		e.find('> div:nth-child(5)').text(sch[3]) // on
		e.find('> div:nth-child(6)').text(sch[4]) // off
		e.on('click', popCus)
		e.appendTo(cus)
	}

	updateSchHight(o)
}

function updateSchHight(o) {
	var def = $('#sch-def')
	var cus = $('#sch-cus')

	//disable all highlight
	def.find('div > div').removeClass('en')
	cus.find('> div').removeClass('en')

	var mid = o.mid;
	if(mid < 0) { // default rule
		var wid = -mid - 10
		var divs = def.find('div > div:nth-child(' + (wid+2) + ')')
		divs.addClass('en')
	} else { // custom rule
		var list = cus.find('> div')
		for(var i=0; i<list.length; i++) {
			var e = $(list[i])
			if(e.attr('mid') == mid) {
				e.addClass('en')
			}
		}
	}
}


function popCus(e) {
	console.log(e, this)
	var t = $(this)
	var mid = t.attr('mid')
	console.log('mid', mid)

	var setSec = function (e, v) {
		for(var i=0; i<3; i++) {
			e.find('input:eq(' + i + ')').val(v[i])
		}
	}

	var popup = $('#cus-rule')
	var w = popup.find('input[n=w]')
	var as = popup.find('.time[n=as]')
	var bs = popup.find('.time[n=bs]')
	var on = popup.find('input[n=on]')
	var of = popup.find('input[n=of]')
	if(mid) {
		popup.attr('mid', mid)
		setSec(as, toTimeStr(t.attr('as')).split(':'))
		setSec(bs, toTimeStr(t.attr('bs')).split(':'))

		w.val(t.attr('w'))
		on.val(t.attr('on'))
		of.val(t.attr('of'))
	} else {
		popup.attr('mid', '')
		w.val('')
		on.val('')
		of.val('')
		setSec(as, ['','',''])
		setSec(bs, ['','',''])
	}

	popup.css('display', 'block')
}

function setCus(cb) {
	var popup = $('#cus-rule')

	var getSec = function (e) {
		var o = []
		for(var i=0; i<3; i++) {
			o.push(e.find('input:eq(' + i + ')').val())
		}
		return o;
	}

	var t2s = function (v) {
		var hh = parseInt(v[0]) || 0
		var mm = parseInt(v[1]) || 0
		var ss = parseInt(v[2]) || 0
		return hh*3600 + mm*60 + ss;
	}

	// TODO: check input
	var mid = popup.attr('mid')
	var w = parseInt(popup.find('input[n=w]').val()) || 0
	var as = getSec(popup.find('.time[n=as]'))
	var bs = getSec(popup.find('.time[n=bs]'))
	var on = parseInt(popup.find('input[n=on]').val()) || 0
	var of = parseInt(popup.find('input[n=of]').val()) || 0
	popup.css('display', 'none')

	console.log('setCus()', w, as, bs, on, of)
	console.log('as', t2s(as))
	console.log('bs', t2s(bs))

	if(w == 7) w = 0
	as = t2s(as) + 1
	bs = t2s(bs) + 1

	var u = '/sch/'
	var parm = ''
	if (mid) {
		u += 'mod?'
		parm += 'i=' + (parseInt(mid) + 1) + '&'
	} else {
		u += 'add?'
	}
	parm += 'w=' + (w+1)
	parm += '&as=' + as
	parm += '&bs=' + bs
	parm += '&on=' + (on+1)
	parm += '&of=' + (of+1)

	setdata(u, parm, function(obj){
		console.log(parm, obj)
		getSchedule()
		if(typeof cb === "function") cb(obj)
	}, function(e){
		console.log('err', parm, e)
		msg('Set Output Rule Error', e.status + ' - ' + e.statusText)
	})
}

function delCus(cb) {
	var popup = $('#cus-rule')
	var mid = popup.attr('mid')
	popup.css('display', 'none')

	console.log('delCus()', mid)

	if (!mid) {
		return
	}
	var parm = 'i=' + (parseInt(mid) + 1)
	setdata('/sch/rm?', parm, function(obj){
		console.log(parm, obj)
		getSchedule()
		if(typeof cb === "function") cb(obj)
	}, function(e){
		console.log('err', parm, e)
		msg('Reome Output Rule Error', e.status + ' - ' + e.statusText)
	})
}

function setDef(cb) {
	var popup = $('#def-rule')

	var w = popup.attr('wid')
	var wm = {
		'All': 8,
		'w7': 1,
		'w1': 2,
		'w2': 3,
		'w3': 4,
		'w4': 5,
		'w5': 6,
		'w6': 7,
	}

	// TODO: check number
	var on = parseInt(popup.find('.param input[n=on]').val()) || 0
	var of = parseInt(popup.find('.param input[n=of]').val()) || 0
	popup.css('display', 'none')

	var parm = 'w=' + wm[w]
	parm += '&on=' + (on+1)
	parm += '&of=' + (of+1)

	setdata('/sch/def?', parm, function(obj){
		console.log('/sch/def', obj)
		getSchedule()
		if(typeof cb === "function") cb(obj)
	}, function(e){
		console.log('/sch/def err', e)
		msg('Set Default Output Error', e.status + ' - ' + e.statusText)
	});
}

function init(){

	// modal
	$('.modal > .content > .header > .close, .modal > .content .cancel.btn').on('click', function(e){
		$('.modal').css('display', 'none')
	})
	$('.modal').on('click', function(e){
		if(e.target == this) $('.modal').css('display', 'none')
	})

	// navbar
	$('.nav > div.btn').on('click', function(e){
		var id = $(this).text().toLowerCase()
		$('.tabs > div.block').css('display', 'none')
		$('#' + id).css('display', 'block')
		if(id == 'schedule') getSchedule()
		if(id == 'settings') getSetting()
		if(id == 'logs') getLogs()
	})

	// bind default setting handler
	$('#sch-def > div:nth-child(1) > div').on('click', function(e){
		e = $(this)
		var popup = $('#def-rule')

		//console.log(popup.find('div.header > h2').text(), e.text())
		var str = e.text()
		if(str == 'week') str = 'All'
		popup.find('div.header > h2').text('Default Output Rule: ' + str)
		popup.css('display', 'block')
		popup.attr('wid', str)
		popup.find('input[n=on]').val(e.attr('on'))
		popup.find('input[n=of]').val(e.attr('of'))
	})
	$('#def-rule .param .primary.btn').on('click', setDef)
	$('#cus-rule .param .primary.btn').on('click', setCus)
	$('#cus-rule .param .danger.btn').on('click', delCus)

	$('#schedule > div > div.btn').on('click', function(e){
		console.log('schedule btn', $(this).attr('do'))
		var act = $(this).attr('do')
		switch (act) {
		case 'load':
			syscall(3, getSchedule)
			break
		case 'save':
			syscall(2)
			break
		}
	})

	$('#settingBtn').on('click', function(e){
		var wm = parseInt($('#wifi-mode').val()) || 3
		var ap_ssid = $('#ap-ssid').val()
		var ap_pwd = $('#ap-pwd').val()
		var hide = ($('#ap-hide').is(':checked')) ? 1 : 0
		var chan = parseInt($('#ap-chan').val()) || 1
		var sta_ssid = $('#sta-ssid').val()
		var sta_pwd = $('#sta-pwd').val()
		var pwr_sleep = parseInt($('#pwr-sleep').val()) || 8

		var new_key = $('#key2').val()
		if (new_key != '') new_key = sha256.hex(new_key);

		console.log('mode', wm)
		console.log('AP', ap_ssid, ap_pwd, hide, chan)
		console.log('STA', sta_ssid, sta_pwd)

		// TODO: dynamic Magic
		var param = 'ESP23333\n'
		param += wm + '\n'
		param += ap_ssid + '\n'
		param += ap_pwd + '\n'
		param += chan + '\n'
		param += hide + '\n'
		param += sta_ssid + '\n'
		param += sta_pwd + '\n'

		param += pwr_sleep + '\n'

		param += new_key + '\n'

		console.log('param', param)
		setSetting(param, function(o){
			// update old key
			$('#key').val($('#key2').val())
			$('#key2').val('').attr('type','password')
			getSetting()
		})
	})
	$('#rebootBtn').on('click', function(e){
		syscall(1)
	})

	var rndStr = function (len) {
		var str = ''
		var char = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
		var charlen = char.length

		for (var i=0; i<len; i++) {
			str += char.charAt(Math.floor(Math.random() * charlen))
		}

		return str
	}
	$('#settings > div > div.btn').on('click', function(e){
		//console.log('settings btn', $(this).attr('do'))
		var kele = $('#key')
		var act = $(this).attr('do')
		switch (act) {
		case 'load':
			kele.val(localStorage.getItem('key'))
			break
		case 'save':
			localStorage.setItem('key', kele.val())
			break
		case 'new':
			$('#key2').attr('type','text').val(rndStr(12))
			break
		}
	})
	$('#key').val(localStorage.getItem('key'))

	// log
	var genChart = function(id, series, style) {
		var chart = new SmoothieChart({responsive:true,millisPerPixel:1000,grid:{millisPerLine:300000},interpolation:'linear',tooltip:true,labels:{fontSize:12},timestampFormatter:SmoothieChart.timeFormatter});
		var canvas = document.getElementById(id);
		chart.addTimeSeries(series, style);
		chart.streamTo(canvas, 500);
	}

	genChart('temp-chart', logs.temp, {lineWidth:2,strokeStyle:'#00ff00'})
	genChart('hum-chart', logs.hum, {lineWidth:2,strokeStyle:'#0088ff',fillStyle:'rgba(0, 136, 255, 0.2)'})
	genChart('press-chart', logs.press, {lineWidth:2,strokeStyle:'#00ff00',fillStyle:'rgba(0, 255, 0, 0.2)'})
}

function getLogs(cb) {
	$.ajax({
	url: url + '/log/all',
	type: 'GET',
	crossDomain: true,
	error: console.log,
	success: function(obj){
		//console.log('/log/all', obj)

		var lines = obj.split('\n')
		var count = parseInt(lines[0])
		var now = new Date().getTime()
		var l = []
		for(var i=1; i<=count; i++) {
			var d = lines[i].split(',')

			var temp = d[1] / 32.0
			var hum = d[2] / 40.0
			var press = (d[3] / 200.0) + 1013.0

			var t = now - 60*1000*(count - i)

			logs.temp.append(t, temp)
			logs.hum.append(t, hum)
			logs.press.append(t, press)
			l.push([t, temp, hum, press])
		}
		//console.log('/log/all', l)

		if(typeof cb === "function") cb(l)
	}})
}

function poll() {
	getStatus(updateSchHight);
	var t = setTimeout(poll, 1000)
}

$(window).on('load', function(e) {
	init()
	poll()
	getSchedule()
})

