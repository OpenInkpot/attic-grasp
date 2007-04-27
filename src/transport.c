#include "common.h"
#include <curl/curl.h>

static CURL *curl;

struct get_handle {
	FILE *file;
	char *name;
	void *addr;
};

int prepare_get()
{
	curl = curl_easy_init();
	if (!curl)
		return -ENOMEM;

	return 0;
}

void done_get()
{
	curl_easy_cleanup(curl);
}

static int file_write_cb(void *buf, size_t size, size_t nmemb, void *stream)
{
	struct get_handle *h = (struct get_handle *)stream;

	if (h && !h->file) {
		DBG("opening %s\n", h->name);
		h->file = fopen(h->name, "w");
		if (!h->file) return -1;
	}
	SAY2(".");
	return fwrite(buf, size, nmemb, h->file);
}

static int mem_write_cb(void *buf, size_t size, size_t nmemb, void *stream)
{
	struct get_handle *h = (struct get_handle *)stream;
	int i;

	if (!stream) return -1;
	for (i = 0; i < nmemb; i++)
		memcpy(h->addr + (i * size),
			buf + (i * size), size);
	SAY2(".");
	return nmemb;
}

int http_get_file(char *url, char *name)
{
	struct get_handle h = { NULL, name, NULL };
	int ret;

	prepare_get();

	SAY("Downloading %s ", url);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &h);

	ret = curl_easy_perform(curl);
	SAY("\n");
	DBG("perform code: %d\n", ret);
	if (ret) goto out_err;

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ret);
	DBG("response code: %d\n", ret);
	if (h.file) fclose(h.file);
	if (ret != 200) goto out_err;

	goto out;
out_err:
	unlink(h.name);
out:
	done_get();

	return ret;
}

int http_get_mem(char *url, char *buf)
{
	struct get_handle h = { NULL, NULL, NULL };

	h.addr = buf;

	prepare_get();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &h);
	done_get();

	return curl_easy_perform(curl);
}

