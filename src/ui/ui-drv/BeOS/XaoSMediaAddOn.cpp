#include <MediaAddOn.h>
#include <Mime.h>
#include <stdio.h>
#include <MediaDefs.h>
#include <File.h>
#include "XaoSProducer.h"
#include "version.h"
#include "be_checkfile.h"
#include "xerror.h"
int linkaddontoo;
#if 0
struct media_format xaos_format={
	B_MEDIA_ENCODED_VIDEO
};
#endif
struct media_format video_format={
	B_MEDIA_RAW_VIDEO
};

static const struct flavor_info flavor={
	"XaoS " XaoS_VERSION,
	"Play XaoS animation files",
	B_BUFFER_PRODUCER | B_FILE_INTERFACE /*| B_CONTROLLABLE*/,
	0,
	0,
	1,  /* FIXME: Possible instances. XaoS is not threaded so we use 1 for now. Change this in future */

	0,
	0,
	/*&xaos_format*/NULL,
	0,
	1,
	&video_format
};
class XaoSAddOn : public BMediaAddOn
{
	public:
		XaoSAddOn(image_id addonID) : BMediaAddOn(addonID) {}
		~XaoSAddOn() {}
		status_t AutoStart(int, BMediaNode **, int32 *, bool *) {return B_ERROR;}
		virtual bool WantsAutoStart(void) {return false;}
		virtual int32 CountFlavors(void) {return 1;}
		virtual status_t GetConfigurationFor(BMediaNode *node, BMessage *cfg) {return B_OK;}	
		virtual status_t GetFlavorAt(int32 c, const flavor_info **outInfo)
		{
			video_format.u.raw_video = media_raw_video_format::wildcard;
			video_format.u.raw_video.interlace = 1;
#if 0
			xaos_format.u.encoded_video = media_encoded_video_format::wildcard;
			xaos_format.u.encoded_video.encoding = ('XaoS';
			xaos_format.u.encoded_video.output.interlace = 1;
#endif
			*outInfo=&flavor;
			if (!c) return B_OK; else return B_ERROR;
		}
		virtual status_t InitCheck(const char **outFailureText)
		{
			x_message("Check!\n");
			*outFailureText=error;
			return B_OK;
		}
		virtual BMediaNode *InstantiateNodeFor(const flavor_info *info, BMessage *config, status_t *outError)
		{
			return new XaoSProducer(NULL, this);
		}
                virtual status_t SniffRef(const entry_ref &file, BMimeType *outMimeType, float *outQuality, int32 *outInternalId)
		{
			BFile node(&file, O_RDONLY);
			if (node.InitCheck() != B_OK) {
				return B_ERROR;
			}
			if (XaoSCheckFile(&node))
			{
				outMimeType->SetType("video/x-xaos-animation");
				*outQuality=(float)1;
				return B_OK;
			}
			return B_ERROR;
		}
		virtual status_t SniffType(const BMimeType &type, float *outQuality, int32 *outInternalID)
		{
			if (!strcmp("video/x-xaos-animation", type.Type()))
			{
				printf("Snifftype!\n");
				*outQuality=(float)1;
				return B_OK;
			} else {
				printf("Unknown!\n");
				*outQuality=(float)0;
				return B_ERROR;
			}
			return B_MEDIA_NO_HANDLER;
		}
	private:
		const char *error;
};
extern "C" {
XaoSAddOn *make_media_addon(image_id addonID)
{
	printf("XaoSAddOn\n");
	return new XaoSAddOn(addonID);
}
}
