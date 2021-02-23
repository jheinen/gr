serve: check_gr.js
	python3 -m http.server

check_gr.js:
	@if [ ! -e "gr.js" ]; then \
	    >&2 echo "Please compile gr.js first (\`make -C js\` in the root directory)."; \
	    exit 1; \
	fi


.PHONY: serve check_gr.js
