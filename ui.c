#include <algo.h>

void decl_img(const char *name, img_t *img)
{
}

void decl_btn(const char *name, void (*cb)(const char *))
{
}

void decl_var_int(const char *name, int *p, int min, int max)
{
}

void decl_var_double(const char *name, double *p, double min, double max)
{
}

static void track_run(const char *_) 
{
   	bRun=1;
}

static void track_stop(const char *_) 
{
   	bRun=0;
}

static void track_continue(const char *_) 
{
   	bPause=0;
}

static void track_pause(const char *_) 
{
	if(DirectT==0)
	{
		DirectT=1;
		mcu_send("192.168.1.3","PDI PDMINPUT=12576;#",21);
	}
   	bPause=1;
}

void ui_init()
{
	decl_btn("RUN", track_run);
	decl_btn("STOP", track_stop);
	decl_btn("Continue", track_continue);
	decl_btn("Pause", track_pause);
}

void testxml()
{
	int rc;
	xmlTextWriterPtr writer;
	xmlDocPtr doc;
	xmlChar *tmp;

	/* 创建一个新的xml Writer，无压缩*/
	writer = xmlNewTextWriterDoc(&doc, 0);
	if (writer == NULL) {        return;    }

	/* 文档声明部分 */
	rc = xmlTextWriterStartDocument(writer, NULL, "ISO-8859-1", NULL);
	if (rc < 0) {      return;    }

	/* 创建第一个元素"EXAMPLE"作为文档的根元素. */
	rc = xmlTextWriterStartElement(writer, (xmlChar*)"EXAMPLE");
	if (rc < 0) {       return;    }

	/* 为EXAMPLE增加一个注释作为子元素，因为xmlTextWriter函数都使用
	 *      * UTF-8的编码，所以这里对中文注释做一个编码转换*/

	/* 假设wchar_t cmt 指向 "这是一个EXAMPLE元素的注释" 的UTF-16串*/
	tmp = "这是一个EXAMPLE";
	rc = xmlTextWriterWriteComment(writer, tmp);
	if (rc < 0) {       return;    }

	/*增加一个新的子元素ORDER*/
	rc = xmlTextWriterStartElement(writer, (xmlChar*)"ORDER");
	if (rc < 0) {       return;    }
	/*为ORDER增加一个子元素，*/
	rc = xmlTextWriterWriteFormatElement(writer, (xmlChar*)"NO", "%d",20);

	/*结束子元素ORDER，直接调用下面的函数即可*/
	rc = xmlTextWriterEndElement(writer);

	/*结束元素EXAMPLE*/
	rc = xmlTextWriterEndElement(writer);

	/*释放xmlWriter的相关资源*/  
	xmlFreeTextWriter(writer);

	/*写XML文档（doc）到文件*/
	xmlSaveFileEnc("test.xml", doc, "UTF-8");
	xmlFreeDoc(doc);




void readxml() 
{
	xmlDoc *doc;
	xmlNode *root;

	doc = xmlReadFile("");

}


