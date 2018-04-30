'use strict';

var url = 'http://192.168.1.116'
var key = '0123456789abcdef'

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

function msg(hdr, str) {
	var e = $('#msg')
	e.find('div.header > h2').text(hdr)
	e.find('.content > p').text(str)
	e.css('display', 'block')
}


var stEle = mkStatus()
var st = null
function mkStatus() {
	var e = $('#status')
	var out = {}
	out.e = e

	var p = ["temp", "hum", "pin", "on", "off", "heap"]
	p.forEach(function(v,k){
		//console.log(k,v)
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
		st = obj
		updateStatusEle(stEle, obj)
		if(cb) cb(obj)
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
		e.temp.text( o.sen[0] / 100.0 )
		e.hum.text( o.sen[1] / 40.0 )
	}
}


var schedule = null
function getSchedule(cb) {
	$.ajax({
	url: url + '/sch/ls',
	type: 'GET',
	crossDomain: true,
	error: console.log,
	success: function(obj){
		console.log('/sch/ls', obj)
		schedule = obj
		updateSchEle(obj)
		if(cb) cb(obj)
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

	setdata(u, parm, key, function(obj){
		console.log(parm, obj)
		getSchedule()
		if(cb) cb(obj)
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
	setdata('/sch/rm?', parm, key, function(obj){
		console.log(parm, obj)
		getSchedule()
		if(cb) cb(obj)
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

	setdata('/sch/def?', parm, key, function(obj){
		console.log('/sch/def', obj)
		getSchedule()
		if(cb) cb(obj)
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
		//console.log(e, this, id)
		$('.tabs > div.block').css('display', 'none')
		$('#' + id).css('display', 'block')
		if(id == 'schedule') getSchedule()
	})

	// bind default setting handler
	$('#sch-def > div:nth-child(1) > div').on('click', function(e){
		//console.log(e, this)
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
	$('#def-rule .param .primary.btn').on('click', function(e){
		//console.log(e, this)
		setDef()
	})

	$('#cus-rule .param .primary.btn').on('click', function(e){
		//console.log(e, this)
		setCus()
	})
	$('#cus-rule .param .danger.btn').on('click', function(e){
		console.log(e, this)
		delCus()
	})
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



function setdata(u, parms, key, cb, errcb) {
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
		var parmsBytes = aesjs.utils.utf8.toBytes(parms);
		var keyBytes = aesjs.utils.utf8.toBytes(key);

		var aesCtr = new aesjs.ModeOfOperation.ctr(keyBytes, new aesjs.Counter(iv));
		var encryptedBytes = aesCtr.encrypt(parmsBytes);

		parms += '&k=' + aesjs.utils.hex.fromBytes(encryptedBytes);

		$.ajax({
		url: url + u + parms,
		type: 'GET',
		crossDomain: true,
		error: function(e){
			console.log('send err', e)
			if(errcb) errcb(obj)
		},
		success: function(obj){
			console.log('send ret', obj)
			if(cb) cb(obj)
		}})
	}})
}

function settest(parms, key, cb) {
	$.ajax({
	url: url + '/token',
	type: 'GET',
	crossDomain: true,
	error: console.log,
	success: function(hex){
		console.log('/token', hex)
		var iv = aesjs.utils.hex.toBytes(hex);
		var parmsBytes = aesjs.utils.utf8.toBytes(parms);
		var keyBytes = aesjs.utils.utf8.toBytes(key);

		var aesCtr = new aesjs.ModeOfOperation.ctr(keyBytes, new aesjs.Counter(iv));
		var encryptedBytes = aesCtr.encrypt(parmsBytes);

		parms = aesjs.utils.hex.fromBytes(encryptedBytes);

		$.ajax({
//		url: url + '/setting?c=' + parms,
		url: url + '/setting',
		data: 'c=' + parms,
		type: 'POST',
		crossDomain: true,
		error: function(e){
			console.log('send err', e)
		},
		success: function(obj){
			console.log('send ret', obj)
			if(cb) cb(obj)
		}})
	}})
}

