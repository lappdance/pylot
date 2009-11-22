# Makefile to build the documentation
HTML_TEMPLATE = templates/basic.txt

dist-docs:
	./doxygen_filter.py ../pilot.h > pilot.h && \
	./doxygen_filter.py ../pilot_limits.h > pilot_limits.h && \
	doxygen Doxyfile
	rm pilot.h
	rm pilot_limits.h

html:
	mkdir -p html/ && \
	rst2html.py --template=$(HTML_TEMPLATE) --cloak-email-addresses \
		tutorial.txt html/tutorial.html && \
	rst2html.py --template=$(HTML_TEMPLATE) --cloak-email-addresses \
		../INSTALL html/install.html

clean:
	rm -rf man
	rm -rf html
