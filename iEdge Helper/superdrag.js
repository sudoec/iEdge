new class SuperDrag {
    constructor() {
        this.isMac = (navigator.platform == "Mac68K") || (navigator.platform == "MacPPC") || (navigator.platform == "Macintosh") || (navigator.platform == "MacIntel");
        this._dic = {};
        this.handle_type = null;
        this.dragEvent = null;
        this.dragInBox = false;
        this.toCancel = false;
        this.search_url = 'https://www.google.com/search?q=%s&ie=utf-8&oe=utf-8'

        document.addEventListener('dragstart', ev => this.dragstart(ev), false);
        document.addEventListener('dragover', ev => this.dragover(ev), false);
        document.addEventListener('dragend', ev => this.dragend(ev), false);
    }

    dragstart(event) {
        this._dic.start_time = new Date().getTime();
        this._dic.startX = event.x;
        this._dic.startY = event.y;
        this.dragEvent = event;
    }

    dragover(event) {
        if (event.altKey || (this.isMac && event.metaKey)) {
            this.toCancel = true;
        }

        if (event.button==0&&event.target.tagName&&((event.target.tagName.toLowerCase()=="input"&&event.target.type=="text")||event.target.tagName.toLowerCase()=="textarea")) {
            this.dragInBox = true
        } else {
            event.preventDefault();
            event.dataTransfer.dropEffect = 'link';
            this.dragInBox = false
        }
        if (!this.dragInBox && !this.toCancel) {
            this._dic.endX = event.x;
            this._dic.endY = event.y;
            let moveX = this._dic.endX - this._dic.startX;
            let moveY = this._dic.endY - this._dic.startY;

            if (Math.sqrt(moveX*moveX, moveY*moveY) < 32) {
                this._dic.timeout = true;
            } else {
                this._dic.timeout = false;
            }

            let keyword = window.getSelection().toString().replace(/(^\s*)|(\s*$)/g, "");
            let urlPattern = /^(http:\/\/www\.|https:\/\/www\.|http:\/\/|https:\/\/)?[a-z0-9]+([\-.]{1}[a-z0-9]+)*\.[a-z]{2,5}(:[0-9]{1,5})?(\/.*)?$/i;
            if (this.dragEvent.srcElement.localName == "a") {
                this._dic['url'] = this.dragEvent.srcElement.href;
                this._dic['active'] = true;
                this._dic['flag'] = 'openTable';
                this.handle_type = "sendMessage";
            } else if (keyword) {
                if (urlPattern.test(keyword)) {
                    if (keyword.substr(0, 4) != 'http') {
                        keyword = "http://" + keyword;
                    }
                    this._dic['url'] = keyword;
                    this._dic['active'] = true;
                    this._dic['flag'] = 'openTable';
                    this.handle_type = "sendMessage";
                } else {
                    this._dic['url'] = this.search_url.replace(/%s/gi, encodeURIComponent(keyword));
                    this._dic['active'] = true;
                    this._dic['flag'] = 'openTable';
                    this.handle_type = "sendMessage";
                }
            } else if (["A"].indexOf(this.dragEvent.srcElement.parentNode.nodeName) != -1) {
                this._dic['url'] = this.dragEvent.srcElement.parentNode.href;
                this._dic['active'] = true;
                this._dic['flag'] = 'openTable';
                this.handle_type = "sendMessage";
            }

        }
    }

    dragend(event) {
        if (this._dic && !this._dic.timeout && !this.toCancel && ["link", "none"].indexOf(this.dragEvent.dataTransfer.dropEffect) != -1) {
            if (this.handle_type == "sendMessage") {
                chrome.runtime.sendMessage(this._dic);
            }
        }
        this.clear_up();
    }

    clear_up() {
        this._dic = {};
        this.handle_type = null;
        this.dragEvent = null;
        this.dragInBox = false;
        this.toCancel = false;
        document.removeEventListener('dragstart', this.dragstart, false);
        document.removeEventListener('dragover', this.dragover, false);
        document.removeEventListener('dragend', this.dragend, false);
    }

}
