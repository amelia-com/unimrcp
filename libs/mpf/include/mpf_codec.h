/*
 * Copyright 2008-2015 Arsen Chaloyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MPF_CODEC_H
#define MPF_CODEC_H

/**
 * @file mpf_codec.h
 * @brief MPF Codec
 */ 

#include "mpf_codec_descriptor.h"

APT_BEGIN_EXTERN_C

/** Codec virtual table declaration */
typedef struct mpf_codec_vtable_t mpf_codec_vtable_t;
/** Codec declaration*/
typedef struct mpf_codec_t mpf_codec_t;

/** Codec */
struct mpf_codec_t {
	/** Codec manipulators (encode, decode, dissect) */
	const mpf_codec_vtable_t     *vtable;
	/** Codec attributes (capabilities) */
	const mpf_codec_attribs_t    *attribs;
	/** Optional static codec descriptor (pt < 96) */
	const mpf_codec_descriptor_t *static_descriptor;
	/** Encoder-specific object */
	void                         *encoder_obj;
	/** Decoder-specific object */
	void                         *decoder_obj;
	/** Memory pool */
	apr_pool_t                   *pool;
};

/** Table of codec virtual methods */
struct mpf_codec_vtable_t {
	/** Virtual open encoder method */
	apt_bool_t (*open_encoder)(mpf_codec_t *codec);
	/** Virtual close encoder method */
	apt_bool_t (*close_encoder)(mpf_codec_t *codec);

	/** Virtual open decoder method */
	apt_bool_t(*open_decoder)(mpf_codec_t *codec);
	/** Virtual close decoder method */
	apt_bool_t(*close_decoder)(mpf_codec_t *codec);

	/** Virtual encode method */
	apt_bool_t (*encode)(mpf_codec_t *codec, const mpf_codec_frame_t *frame_in, mpf_codec_frame_t *frame_out);
	/** Virtual decode method */
	apt_bool_t (*decode)(mpf_codec_t *codec, const mpf_codec_frame_t *frame_in, mpf_codec_frame_t *frame_out);

	/** Virtual dissect method */
	apt_bool_t (*dissect)(mpf_codec_t *codec, void **buffer, apr_size_t *size, mpf_codec_frame_t *frame);

	/** Virtual fill with silence method */
	apt_bool_t (*fill)(mpf_codec_t *codec, mpf_codec_frame_t *frame_out);

	/** Virtual format matching method */
	mpf_codec_format_match_f match_formats;
};

/**
 * Create codec.
 * @param vtable the table of virtual mthods
 * @param attribs the codec attributes
 * @param descriptor the codec descriptor
 * @param pool the pool to allocate memory from
 */
static APR_INLINE mpf_codec_t* mpf_codec_create(
									const mpf_codec_vtable_t *vtable, 
									const mpf_codec_attribs_t *attribs, 
									const mpf_codec_descriptor_t *descriptor, 
									apr_pool_t *pool)
{
	mpf_codec_t *codec = (mpf_codec_t*)apr_palloc(pool,sizeof(mpf_codec_t));
	codec->vtable = vtable;
	codec->attribs = attribs;
	codec->static_descriptor = descriptor;
	codec->encoder_obj = NULL;
	codec->decoder_obj = NULL;
	codec->pool = pool;
	return codec;
}

/**
 * Clone codec.
 * @param src_codec the source (original) codec to clone
 * @param pool the pool to allocate memory from
 */
static APR_INLINE mpf_codec_t* mpf_codec_clone(mpf_codec_t *src_codec, apr_pool_t *pool)
{
	mpf_codec_t *codec = (mpf_codec_t*)apr_palloc(pool,sizeof(mpf_codec_t));
	codec->vtable = src_codec->vtable;
	codec->attribs = src_codec->attribs;
	codec->static_descriptor = src_codec->static_descriptor;
	codec->encoder_obj = NULL;
	codec->decoder_obj = NULL;
	codec->pool = pool;
	return codec;
}

/** Open encoder */
static APR_INLINE apt_bool_t mpf_codec_encoder_open(mpf_codec_t *codec)
{
	apt_bool_t rv = TRUE;
	if(codec->vtable->open_encoder) {
		rv = codec->vtable->open_encoder(codec);
	}
	return rv;
}

/** Close encoder */
static APR_INLINE apt_bool_t mpf_codec_encoder_close(mpf_codec_t *codec)
{
	apt_bool_t rv = TRUE;
	if(codec->vtable->close_encoder) {
		rv = codec->vtable->close_encoder(codec);
	}
	return rv;
}

/** Open decoder */
static APR_INLINE apt_bool_t mpf_codec_decoder_open(mpf_codec_t *codec)
{
	apt_bool_t rv = TRUE;
	if (codec->vtable->open_decoder) {
		rv = codec->vtable->open_decoder(codec);
	}
	return rv;
}

/** Close decoder */
static APR_INLINE apt_bool_t mpf_codec_decoder_close(mpf_codec_t *codec)
{
	apt_bool_t rv = TRUE;
	if (codec->vtable->close_decoder) {
		rv = codec->vtable->close_decoder(codec);
	}
	return rv;
}

/** Encode codec frame */
static APR_INLINE apt_bool_t mpf_codec_encode(mpf_codec_t *codec, const mpf_codec_frame_t *frame_in, mpf_codec_frame_t *frame_out)
{
	apt_bool_t rv = TRUE;
	if(codec->vtable->encode) {
		rv = codec->vtable->encode(codec,frame_in,frame_out);
	}
	return rv;
}

/** Decode codec frame */
static APR_INLINE apt_bool_t mpf_codec_decode(mpf_codec_t *codec, const mpf_codec_frame_t *frame_in, mpf_codec_frame_t *frame_out)
{
	apt_bool_t rv = TRUE;
	if(codec->vtable->decode) {
		rv = codec->vtable->decode(codec,frame_in,frame_out);
	}
	return rv;
}

/** Dissect codec frame (navigate through codec frames in a buffer, which may contain multiple frames) */
static APR_INLINE apt_bool_t mpf_codec_dissect(mpf_codec_t *codec, void **buffer, apr_size_t *size, mpf_codec_frame_t *frame)
{
	apt_bool_t rv = TRUE;
	if(codec->vtable->dissect) {
		/* custom dissector for codecs like G.729, G.723 */
		rv = codec->vtable->dissect(codec,buffer,size,frame);
	}
	else {
		/* default dissector */
		if(*size >= frame->size && frame->size) {
			memcpy(frame->buffer,*buffer,frame->size);
			
			*buffer = (apr_byte_t*)*buffer + frame->size;
			*size = *size - frame->size;
		}
		else {
			rv = FALSE;
		}
	}
	return rv;
}

/** Fill codec frame with silence */
static APR_INLINE apt_bool_t mpf_codec_fill(mpf_codec_t *codec, mpf_codec_frame_t *frame_out)
{
	apt_bool_t rv = TRUE;
	if(codec->vtable->fill) {
		rv = codec->vtable->fill(codec,frame_out);
	}
	else {
		memset(frame_out->buffer,0,frame_out->size);
	}
	return rv;
}

APT_END_EXTERN_C

#endif /* MPF_CODEC_H */
