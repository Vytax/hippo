var PDFBrowser = Class.create({
    initialize: function(hash) {
        this.pageCount = jsB.PDFpageCount(hash);
        this.pageNum = 1;
        this.hash = hash;
        
        pdfarea = $(hash);
        pdfarea.hide();

        enmedia = pdfarea.up('en-media');

        this.pdfpreview = new Element('div', {'class': 'pdfpreview'});
        enmedia.appendChild(this.pdfpreview);

        controls = new Element('div', {'class': 'pdfcontrols'});
        this.pdfpreview.appendChild(controls);

        controls.appendChild(new Element('span').update(jsB.getResourceFileName(hash)));

        close = new Element('img', {src: 'qrc:///img/dialog-close.png', 'class': 'pointer pdf_close', 'hint': 'Close Preview'});
        close.observe('click', this.removePreview.bind(this));
        controls.appendChild(close);
        setHintEvent(close);

        pdfcontainer = new Element('div', {'class': 'pdfcontainer'});
        this.pdfpreview.appendChild(pdfcontainer);

        this.image = new Element('img', {'src': 'pdf://' + hash + '/0', 'class': 'loading pdfpage'});
        pdfcontainer.appendChild(this.image);
        this.image.observe('load', this.imageLoadEvent.bind(this));
        this.image.observe('mousemove', this.mouseMoveEvent.bind(this));
        this.image.observe('mousedown', this.mouseDownEvent.bind(this));
        this.image.observe('mouseup', this.mouseUpEvent.bind(this));
        this.image.observe('mouseover', this.mouseUpEvent.bind(this));
        this.image.observe('mousewheel', this.mouseWheelEvent.bind(this));

        this.controls = new Element('div', {'class': 'pdfcontrols'});
        this.pdfpreview.appendChild(this.controls);
        this.controls.hide();

        ZoomIn = new Element('img', {src: 'qrc:///img/zoom-in.png', 'class': 'pointer', 'hint': 'Zoom In'});
        ZoomIn.observe('click', this.zoomInEvent.bind(this));
        this.controls.appendChild(ZoomIn);
        setHintEvent(ZoomIn);

        ZoomOut = new Element('img', {src: 'qrc:///img/zoom-out.png', 'class': 'pointer', 'hint': 'Zoom Out'});
        ZoomOut.observe('click', this.zoomOutEvent.bind(this));
        this.controls.appendChild(ZoomOut);
        setHintEvent(ZoomOut);

        pdfpagination = new Element('span', {'class': 'pdfpagination'});
        this.controls.appendChild(pdfpagination);

        this.previous = new Element('img', {src: 'qrc:///img/go-previous-view.png', 'class': 'pointer', 'hint': 'Previous page'});
        this.previous.observe('click', this.previousPageEvent.bind(this));
        pdfpagination.appendChild(this.previous);
        setHintEvent(this.previous);

        pdfpagination.appendChild(new Element('span').update('Page: '));

        this.pgNum = new Element('span', {'class': 'pointer'}).update(this.pageNum);
        this.pgNum.observe('click', this.pdfPageClickEvent.bind(this));
        pdfpagination.appendChild(this.pgNum);

        pdfpagination.appendChild(new Element('span').update('/' + this.pageCount));

        this.next = new Element('img', {src: 'qrc:///img/go-next-view.png', 'class': 'pointer', 'hint': 'Next page'});
        this.next.observe('click', this.nextPageEvent.bind(this));
        pdfpagination.appendChild(this.next);
        setHintEvent(this.next);
        
        this.updatePage();
    },
    pdfPageClickEvent: function(event) {
        page = jsB.requestPDFpageNumber(this.pageCount, this.pageNum);
        if (page > 0) {
            this.pageNum = page;
            this.updatePage();
        }
    },
    previousPageEvent: function(event) {
        if (this.pageNum > 1) {
            this.pageNum -= 1;
            this.updatePage();
        }
    },
    nextPageEvent: function(event) {
        if (this.pageNum < this.pageCount) {
            this.pageNum += 1;
            this.updatePage();
        }
    },
    updatePage: function(event) {
        this.pgNum.update(this.pageNum);
        if (this.pageNum == 1) {
            this.previous.setOpacity(0.3);
        }
        else {
            this.previous.setOpacity(1);
        }
        
        if (this.pageNum == this.pageCount) {
            this.next.setOpacity(0.3);
        }
        else {
            this.next.setOpacity(1);
        }
        this.image.setAttribute('src', 'pdf://' + this.hash + '/' + (this.pageNum - 1));
    },
    imageLoadEvent: function(event) {
        this.drag = false;
        this.draggingRectOffsetX = 0;
        this.draggingRectOffsetY = 0;
        this.left = this.image.cumulativeOffset().left;
        this.top = this.image.cumulativeOffset().top;
        this.x = 0;
        this.y = 0;

        w1 = this.image.getWidth();
        w2 = this.image.up('div').getWidth();
        this.scale = w2 / w1;
        this.render();
        this.image.setStyle({
            '-webkit-transform-origin': '0px 0px'
        });
        this.controls.show();
    },
    render: function() {
        this.image.setStyle({
            '-webkit-transform': 'scale(' + this.scale + ') translate(' + this.x + 'px, ' + this.y + 'px)'
        });
    },
    zoom: function(scale) {
        oldScale = this.scale;
        this.scale = this.scale + scale;
        if (this.scale < 0.1) {
            this.scale = 0.1;
        }
        if (this.scale > 10) {
            this.scale = 10;
        }
        scaleDiff = this.scale / oldScale;
        this.x = Math.round(this.x * scaleDiff);
        this.y = Math.round(this.y * scaleDiff);
        this.render();
    },
    zoomInEvent: function(event) {
        this.zoom(0.1);
        Event.stop(event);
    },
    zoomOutEvent: function(event) {
        this.zoom(-0.1);
        Event.stop(event);
    },
    mouseMoveEvent: function(event) {
        if (this.drag) {
            this.x = Math.round((Event.pointerX(event) - this.left) / this.scale - this.draggingRectOffsetX);
            this.y = Math.round((Event.pointerY(event) - this.top) / this.scale - this.draggingRectOffsetY);
            this.render();
        }
    },
    mouseDownEvent: function(event) {
        this.draggingRectOffsetX = Math.round((Event.pointerX(event) - this.left) / this.scale) - this.x;
        this.draggingRectOffsetY = Math.round((Event.pointerY(event) - this.top) / this.scale) - this.y;
        this.drag = true;
        Event.stop(event);
    },
    mouseUpEvent: function(event) {
        this.drag = false;
    },
    mouseWheelEvent: function(event) {
        if (event.wheelDelta > 0) {
            this.zoom(0.1);
        } else {
            this.zoom(-0.1);
        }
        Event.stop(event);
    },
    removePreview: function(event) {
        this.pdfpreview.remove();
        $(this.hash).show();
        delete this;
    }
});
