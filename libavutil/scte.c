// #include <stdio.h>
#include "scte.h"


AvailDescriptor* init_avail_descriptor(uint8_t** payload_ptr, int* payload_size)
{
	uint8_t *payload = (uint8_t *)*payload_ptr;
	AvailDescriptor* avail_descriptor = (AvailDescriptor *)malloc(sizeof(AvailDescriptor));
	avail_descriptor->provider_avail_id = ((uint32_t) *payload++ & 0xff) << 24; 
	avail_descriptor->provider_avail_id |= ((uint32_t) *payload++ & 0xff) << 16; 
	avail_descriptor->provider_avail_id |= ((uint32_t) *payload++ & 0xff) << 8; 
	avail_descriptor->provider_avail_id |= ((uint32_t) *payload++ & 0xff); 
	(*payload_size) += 4;
	*payload_ptr = payload;
	return avail_descriptor;
}

DTMFDescriptor* init_dtmf_descriptor(uint8_t** payload_ptr, int* payload_size)
{
	int i;
	uint8_t *payload = (uint8_t *)*payload_ptr;
	DTMFDescriptor* dtmf_descriptor = (DTMFDescriptor *)malloc(sizeof(DTMFDescriptor));
	dtmf_descriptor->preroll = *payload++;
	dtmf_descriptor->dtmf_count = (*payload >> 5) & 0x07;
	dtmf_descriptor->reserved = (*payload++ & 0x1f);
	(*payload_size) += 2;
	dtmf_descriptor->DTMF_char = (uint8_t *)malloc(dtmf_descriptor->dtmf_count * sizeof(uint8_t));
	for (i=0;i<dtmf_descriptor->dtmf_count; i++)
	{
		dtmf_descriptor->DTMF_char[i] = *payload++;
		(*payload_size)++;
	}
	*payload_ptr = payload;
	return dtmf_descriptor;
}

SegmentationComponent init_segmentation_component(uint8_t** payload_ptr, int* payload_size)
{
	uint8_t *payload = (uint8_t *)*payload_ptr;
	SegmentationComponent seg_component;
	seg_component.component_tag = *payload++;
	seg_component.reserved = (*payload >> 1) & 0x7f;
	
	seg_component.pts_offset = ((uint64_t)(*payload++ & 0x01) << 32); 
	seg_component.pts_offset |= ((uint64_t) *payload++ & 0xff) << 24; 
	seg_component.pts_offset |= ((uint64_t) *payload++ & 0xff) << 16; 
	seg_component.pts_offset |= ((uint64_t) *payload++ & 0xff) << 8; 
	seg_component.pts_offset |= ((uint64_t) *payload++ & 0xff); 
	(*payload_size) += 6;
	*payload_ptr = payload;
	return seg_component;
}

SegmentationDescriptor* init_segmentation_descriptor(uint8_t** payload_ptr, int *payload_size)
{
	int i=0;
	uint8_t *payload = (uint8_t *)*payload_ptr;
	SegmentationDescriptor *seg_descriptor = (SegmentationDescriptor *)malloc(sizeof(SegmentationDescriptor));
	seg_descriptor->segmentation_event_id = ((uint64_t) *payload++ & 0xff) << 24; 
	seg_descriptor->segmentation_event_id |= ((uint64_t) *payload++ & 0xff) << 16; 
	seg_descriptor->segmentation_event_id |= ((uint64_t) *payload++ & 0xff) << 8; 
	seg_descriptor->segmentation_event_id |= ((uint64_t) *payload++ & 0xff);
	(*payload_size) += 4;
	seg_descriptor->segmentation_event_cancel_indicator = (*payload >> 7) & 0x01;
	seg_descriptor->reserved = (*payload++ & 0x7f);
	seg_descriptor->program_segmentation_flag = (*payload >> 7) & 0x01;
	seg_descriptor->segmentation_duration_flag = (*payload >> 6) & 0x01;
	seg_descriptor->delivery_not_restricted_flag = (*payload >> 5) & 0x01;
	if (seg_descriptor->delivery_not_restricted_flag == 0)
	{
		seg_descriptor->web_delivery_allowed_flag = (*payload >> 4) & 0x01; // 1 bit 0x01
		seg_descriptor->no_regional_blackout_flag = (*payload >> 3) & 0x01; // 1 bit 0x01
		seg_descriptor->archive_allowed_flag = (*payload >> 2) & 0x01; // 1 bit 0x01
		seg_descriptor->device_restrictions = *payload++ & 0x03; // 2 bit 0x03
	}
	else
	{
		seg_descriptor->reserved1 = *payload++ & 0x1f;
	}
	(*payload_size) += 2;
	if (seg_descriptor->program_segmentation_flag == 0)
	{
		seg_descriptor->component_count = *payload++;
		(*payload_size)++;
		seg_descriptor->segmentation_components = 
			(SegmentationComponent *)malloc(seg_descriptor->component_count * sizeof(SegmentationComponent));
		for (i=0; i< seg_descriptor->component_count; i++)
		{
			seg_descriptor->segmentation_components[i] = init_segmentation_component(&payload, payload_size);
		}
	}
	if (seg_descriptor->segmentation_duration_flag == 1)
	{
		seg_descriptor->segmentation_duration = ((uint64_t) *payload++ & 0xff) << 32;
		seg_descriptor->segmentation_duration |= ((uint64_t) *payload++ & 0xff) << 24;
		seg_descriptor->segmentation_duration |= ((uint64_t) *payload++ & 0xff) << 16;
		seg_descriptor->segmentation_duration |= ((uint64_t) *payload++ & 0xff) << 8;
		seg_descriptor->segmentation_duration |= ((uint64_t) *payload++ & 0xff); 
		(*payload_size) += 5;
	}
	seg_descriptor->segmentation_upid_type = *payload++;
	seg_descriptor->segmentation_upid_length = *payload++;
	(*payload_size) += 2;
	seg_descriptor->segmentation_upid_data = (uint8_t *)malloc(seg_descriptor->segmentation_upid_length *sizeof(uint8_t)) ;
	for (i=0; i<seg_descriptor->segmentation_upid_length; i++)
	{
		seg_descriptor->segmentation_upid_data[i] = *payload++;
		(*payload_size)++;
	}
	seg_descriptor->segmentation_type_id = *payload++;
	seg_descriptor->segment_num = *payload++;
	seg_descriptor->segments_expected = *payload++;
	(*payload_size) += 3;
	if (seg_descriptor->segmentation_type_id == 0x34 || seg_descriptor->segmentation_type_id== 0x36)
	{
		seg_descriptor->sub_segment_num = *payload++;
		seg_descriptor->sub_segments_expected = *payload++;
		(*payload_size) += 2;
	}
	*payload_ptr = payload;
	return seg_descriptor;
}

TimeDescriptor* init_time_descriptor(uint8_t** payload_ptr, int* payload_size)
{
	uint8_t *payload = (uint8_t *)*payload_ptr;
	TimeDescriptor * time_descriptor = (TimeDescriptor *)malloc(sizeof(TimeDescriptor));
	time_descriptor->TAI_seconds = ((uint64_t) *payload++ & 0xff) << 40;
	time_descriptor->TAI_seconds |= ((uint64_t) *payload++ & 0xff) << 32;
	time_descriptor->TAI_seconds |= ((uint64_t) *payload++ & 0xff) << 24;
	time_descriptor->TAI_seconds |= ((uint64_t) *payload++ & 0xff) << 16;
	time_descriptor->TAI_seconds |= ((uint64_t) *payload++ & 0xff) << 8; 
	time_descriptor->TAI_seconds |= ((uint64_t) *payload++ & 0xff); 
	time_descriptor->TAI_ns = ((uint32_t) *payload++ & 0xff) << 24;
	time_descriptor->TAI_ns |= ((uint32_t) *payload++ & 0xff) << 16;
	time_descriptor->TAI_ns |= ((uint32_t) *payload++ & 0xff) << 8; 
	time_descriptor->TAI_ns |= ((uint32_t) *payload++ & 0xff); 
	time_descriptor->UTC_offset = ((uint16_t) *payload++ & 0xff) << 8;
	time_descriptor->UTC_offset = ((uint16_t) *payload++ & 0xff);
	(*payload_size) += 12;
	*payload_ptr = payload;
	return time_descriptor;

}

SpliceDescriptor init_splice_descriptor(uint8_t** payload_ptr, int* payload_size)
{
	int i =0;
	uint8_t *payload = (uint8_t *)*payload_ptr;
	SpliceDescriptor splice_descriptor;
	splice_descriptor.splice_descriptor_tag = *payload++;
	splice_descriptor.descriptor_length = *payload++;
	splice_descriptor.identifier = ((uint32_t) *payload++ & 0xff) << 24;
	splice_descriptor.identifier |= ((uint32_t) *payload++ & 0xff) << 16;
	splice_descriptor.identifier |= ((uint32_t) *payload++ & 0xff) << 8; 
	splice_descriptor.identifier |= ((uint32_t) *payload++ & 0xff); 
	(*payload_size) += 6;

	switch (splice_descriptor.splice_descriptor_tag)
	{
	case AVAIL_DESCRIPTOR:
		splice_descriptor.avail_descriptor = init_avail_descriptor(&payload, payload_size);
		break;
	case DTMF_DESCRIPTOR:
		splice_descriptor.dtmf_descriptor = init_dtmf_descriptor(&payload, payload_size);
		break;
	case SEGMENTATION_DESCRIPTOR:
		av_log(NULL, AV_LOG_DEBUG,"Read Segmentation Descriptor");
		splice_descriptor.segmentation_descriptor = init_segmentation_descriptor(&payload, payload_size);
		break;
	case TIME_DESCRIPTOR:
		splice_descriptor.time_descriptor = init_time_descriptor(&payload, payload_size);
		break;
	default:
		av_log(NULL, AV_LOG_ERROR,"Descriptor Not Implemented");
		splice_descriptor.raw_bytes = (uint8_t *)malloc(splice_descriptor.descriptor_length * sizeof(uint8_t));

		for (i=0;i<splice_descriptor.descriptor_length;i++)
		{
			splice_descriptor.raw_bytes[i] = *payload++;
			(*payload_size)++;
		}
	}
	
	*payload_ptr = payload;
	return splice_descriptor;
	
}

BreakDuration init_break_duration(uint8_t** payload_ptr, int payload_size)
{
	uint8_t *payload = (uint8_t *)*payload_ptr;
	BreakDuration break_duration;
	break_duration.auto_return = (*payload >> 7) & 0x01;
	break_duration.reserved = (*payload >> 1) & 0x3f;
	break_duration.duration = ((uint64_t)(*payload++ & 0x01) << 32); 
	break_duration.duration |= ((uint64_t) *payload++ & 0xff) << 24; 
	break_duration.duration |= ((uint64_t) *payload++ & 0xff) << 16; 
	break_duration.duration |= ((uint64_t) *payload++ & 0xff) << 8; 
	break_duration.duration |= ((uint64_t) *payload++ & 0xff); 
	*payload_ptr = payload;
	return break_duration;
}

SpliceTime init_splice_time(uint8_t** payload_ptr, int payload_size)
{
	uint8_t *payload = (uint8_t *)*payload_ptr;
	SpliceTime splice_time;
	splice_time.time_specified_flag = (*payload >> 7) & 0x01;
	if (splice_time.time_specified_flag == 0)
	{
		splice_time.reserved = *payload++ & 0x3f;
		*payload_ptr = payload;
		return splice_time;
	}
	splice_time.reserved = (*payload >> 1) & 0x3f;
	splice_time.pts_time = ((uint64_t)(*payload++ & 0x01) << 32); 
	splice_time.pts_time |= ((uint64_t) *payload++ & 0xff) << 24; 
	splice_time.pts_time |= ((uint64_t) *payload++ & 0xff) << 16; 
	splice_time.pts_time |= ((uint64_t) *payload++ & 0xff) << 8; 
	splice_time.pts_time |= ((uint64_t) *payload++ & 0xff); 
	*payload_ptr = payload;
	return splice_time;

}

TimeSignal *init_time_signal(uint8_t** payload_ptr, int payload_size)
{
	uint8_t *payload = (uint8_t *)*payload_ptr;
	TimeSignal *time_signal = (TimeSignal *)malloc(sizeof(TimeSignal));
	time_signal->splice_time = init_splice_time(&payload, payload_size);
	*payload_ptr = payload;
	return time_signal;
}

SpliceInsert *init_splice_insert(uint8_t** payload_ptr, int payload_size)
{
	uint8_t *payload = (uint8_t *)*payload_ptr;
	SpliceInsert *splice_insert = (SpliceInsert*) malloc(sizeof (SpliceInsert));
	
	splice_insert->splice_event_id = ((*payload++) << 24); 
	splice_insert->splice_event_id |= ((*payload++) << 16); 
	splice_insert->splice_event_id |= ((*payload++) << 8); 
	splice_insert->splice_event_id |= (*payload++); 
	splice_insert->splice_event_cancel_indicator = (*payload >> 7) & 0x01;
	splice_insert->reserved = (*payload++ & 0x7f); 

	if (splice_insert->splice_event_cancel_indicator == 0)
	{
		splice_insert->out_of_network_indicator = (*payload >> 7) & 0x01;
		splice_insert->program_splice_flag = (*payload >> 6) & 0x01;
		splice_insert->duration_flag = (*payload >> 5) & 0x01;
		splice_insert->splice_immediate_flag = (*payload >> 4) & 0x01;
		splice_insert->reserved1 = (*payload++ & 0x0f); 
		if (splice_insert->program_splice_flag == 1 && splice_insert->splice_immediate_flag==0)
		{
			splice_insert->single_splice_time = init_splice_time(&payload, payload_size);
		}
		if (splice_insert->program_splice_flag == 0)
		{
			av_log(NULL, AV_LOG_ERROR,"Not Implemented");
		}
		if (splice_insert->duration_flag==1)
		{
			splice_insert->break_duration = init_break_duration(&payload, payload_size);
		}
		splice_insert->unique_program_id = *payload++ << 8; 
		splice_insert->unique_program_id |= (*payload++); 
		splice_insert->avail_num = *payload++; 
		splice_insert->avails_expected = *payload++; 
	}
	*payload_ptr = payload;

	return splice_insert;
}

SCTEPacket init_scte_packet(uint8_t *payload, int payload_size)
{
	SCTEPacket p;
	int descriptor_payload_length =0;
	p.modified = 0;
	p.init_payload = payload;
	p.payload_size = payload_size;
	// printf("Table Id %d\n", *payload);
	p.table_id = *payload++; 
	// printf("Next %d\n", p.table_id);
	p.section_syntax_indicator = (*payload >> 7) & 0x01;
	p.private_indicator = (*payload >> 6) & 0x01;
	p.reserved = (*payload>>4) & 0x03;
	p.section_length = (((*payload++) & 0x0f) << 8) & 0x0fff; 
	p.section_length = (p.section_length | *payload++) & 0x0fff; 
	p.protocol_version = *payload++; 
	p.encrypted_packet = (*payload >> 7) & 0x01;
	p.encryption_algorithm = (*payload >> 1) & 0x2f;
	p.pts_adjustment = (uint64_t)(*payload++ & 0x01) << 32; 
	p.pts_adjustment |= ((*payload++) << 24); 
	p.pts_adjustment |= ((*payload++) << 16); 
	p.pts_adjustment |= ((*payload++) << 8); 
	p.pts_adjustment |= (*payload++); 
	p.cw_index = *payload++; 
	p.tier = (*payload++) << 4; 
	p.tier |= (*payload >> 4);
	p.splice_command_length = (*payload++ & 0x0f)<<8; 
	p.splice_command_length |= *payload++; 
	p.splice_command_type = *payload++; 
	switch (p.splice_command_type)
	{
	case SPLICE_NULL:
		av_log(NULL, AV_LOG_DEBUG,"Encountered Splice NULL \n");
		return p;
		break;
	case SPLICE_INSERT:
		av_log(NULL, AV_LOG_DEBUG,"Encountered Splice Insert\n");
		p.splice_insert = init_splice_insert(&payload, payload_size);
		break;
	case TIME_SIGNAL:
		av_log(NULL, AV_LOG_DEBUG, "Encountered Time Signal\n");
		p.time_signal = init_time_signal(&payload, payload_size);
		break;
	case BANDWIDTH_RESERVATION:
		av_log(NULL, AV_LOG_DEBUG,"Splice Command Type Bandwidth Reservation Not Implemented %d\n", p.splice_command_type);
		return p;
	case SPLICE_SCHEDULE:
		av_log(NULL, AV_LOG_DEBUG,"Splice Command Type Splice Schedule Not Implemented %d\n", p.splice_command_type);
		return p;
	case PRIVATE_COMMAND:
		av_log(NULL, AV_LOG_DEBUG,"Splice Command Type Private Command Not Implemented %d\n", p.splice_command_type);
		return p;
	default:
		av_log(NULL, AV_LOG_DEBUG,"Splice Command Type Not Implemented %d\n", p.splice_command_type);
		return p;
	}
	p.descriptor_loop_length = (*payload++ << 8); 
	p.descriptor_loop_length |= *payload++; 
	p.descriptor_count = 0;
	if (p.descriptor_loop_length !=0){
		p.descriptor_count++;
		p.splice_descriptor = (SpliceDescriptor *)malloc(p.descriptor_count*sizeof(SpliceDescriptor));
		p.splice_descriptor[0] = init_splice_descriptor(&payload, &descriptor_payload_length);
		while (descriptor_payload_length < p.descriptor_loop_length)
		{
			
			p.descriptor_count++;
			p.splice_descriptor = (SpliceDescriptor *)realloc(p.splice_descriptor,p.descriptor_count*sizeof(SpliceDescriptor));
			p.splice_descriptor[p.descriptor_count-1] = init_splice_descriptor(&payload, &descriptor_payload_length);
		}
	}
	p.crc_32 = ((*payload++) << 24);  
	p.crc_32 |= ((*payload++) << 16); 
	p.crc_32 |= ((*payload++) << 8); 
	p.crc_32 |= (*payload++); 
	return  p;
}

void print_splice_descriptor(SpliceDescriptor splice_descriptor)
{
	int i;
	printf("splice_descriptor_tag %d\n", splice_descriptor.splice_descriptor_tag);
	printf("descriptor_length %d\n", splice_descriptor.descriptor_length);
	printf("identifier %d\n", splice_descriptor.identifier);
	switch (splice_descriptor.splice_descriptor_tag)
	{
	case AVAIL_DESCRIPTOR:
		printf("AVAIL Descriptor\n");
		printf("AVAIL Descriptor provider_avail_id %d\n", splice_descriptor.avail_descriptor->provider_avail_id);
		break;
	case DTMF_DESCRIPTOR:
		printf("DTMF Descriptor\n");
		printf("DTMF Descriptor preroll %d \n", splice_descriptor.dtmf_descriptor->preroll);
		printf("DTMF Descriptor dtmf_count %d \n", splice_descriptor.dtmf_descriptor->dtmf_count);
		printf("DTMF Descriptor reserved %d \n", splice_descriptor.dtmf_descriptor->reserved);
		printf("DTMF Char Payload ");
		for (i=0;i<splice_descriptor.dtmf_descriptor->dtmf_count; i++)
			printf("%x ", splice_descriptor.dtmf_descriptor->DTMF_char[i]);
		printf("\n");
		break;
	case SEGMENTATION_DESCRIPTOR:
		printf("Segmentation Descriptor \n");
		printf("Segmentation_event_id %d\n",splice_descriptor.segmentation_descriptor->segmentation_event_id);
		printf("segmentation_event_cancel_indicator %d\n",splice_descriptor.segmentation_descriptor->segmentation_event_cancel_indicator);
		printf("reserved %d\n",splice_descriptor.segmentation_descriptor->reserved);
		if (splice_descriptor.segmentation_descriptor->segmentation_event_cancel_indicator == 0)
		{
			printf("program_segmentation_flag %d\n",splice_descriptor.segmentation_descriptor->program_segmentation_flag);
			printf("segmentation_duration_flag %d\n",splice_descriptor.segmentation_descriptor->segmentation_duration_flag);
			printf("delivery_not_restricted_flag %d\n",splice_descriptor.segmentation_descriptor->delivery_not_restricted_flag);
			if (splice_descriptor.segmentation_descriptor->delivery_not_restricted_flag == 0)
			{
				printf("web_delivery_allowed_flag %d\n",splice_descriptor.segmentation_descriptor->web_delivery_allowed_flag);
				printf("no_regional_blackout_flag %d\n",splice_descriptor.segmentation_descriptor->no_regional_blackout_flag);
				printf("archive_allowed_flag %d\n",splice_descriptor.segmentation_descriptor->archive_allowed_flag);
				printf("device_restrictions %d\n",splice_descriptor.segmentation_descriptor->device_restrictions);
			}
			else
			{
				printf("reserved1 %d\n",splice_descriptor.segmentation_descriptor->reserved1);
			}
			if (splice_descriptor.segmentation_descriptor->program_segmentation_flag == 0)
			{
				printf("component_count %d\n",splice_descriptor.segmentation_descriptor->component_count);	
				for (i=0; i< splice_descriptor.segmentation_descriptor->component_count; i++)
				{
					printf("Component No : %d\n", i);
					printf("component_tag %d\n",splice_descriptor.segmentation_descriptor->segmentation_components[i].component_tag);
					printf("reserved %d\n",splice_descriptor.segmentation_descriptor->segmentation_components[i].reserved);
					printf("pts_offset %llu\n",splice_descriptor.segmentation_descriptor->segmentation_components[i].pts_offset);
				}
			}
			if (splice_descriptor.segmentation_descriptor->segmentation_duration_flag == 1)
			printf("segmentation_duration %llu\n",splice_descriptor.segmentation_descriptor->segmentation_duration);
			printf("segmentation_upid_type %d\n",splice_descriptor.segmentation_descriptor->segmentation_upid_type);
			printf("segmentation_upid_length %d\n",splice_descriptor.segmentation_descriptor->segmentation_upid_length);
			printf("UPID Data\n");
			for (i=0;i<splice_descriptor.segmentation_descriptor->segmentation_upid_length;i++)
			{
				printf("%x ", splice_descriptor.segmentation_descriptor->segmentation_upid_data[i]);
			}
			printf("\n");
			printf("segmentation_type_id %d\n",splice_descriptor.segmentation_descriptor->segmentation_type_id);
			printf("segment_num %d\n",splice_descriptor.segmentation_descriptor->segment_num);
			printf("segments_expected %d\n",splice_descriptor.segmentation_descriptor->segments_expected);
			if (splice_descriptor.segmentation_descriptor->segmentation_type_id == 0x34 || splice_descriptor.segmentation_descriptor->segmentation_type_id == 0x36)
			{
				printf("sub_segment_num %d\n",splice_descriptor.segmentation_descriptor->sub_segment_num);
				printf("sub_segments_expected %d\n",splice_descriptor.segmentation_descriptor->sub_segments_expected);
			}
		}
		break;
	default:
		break;
	}
}
void print_scte_packet(SCTEPacket pt)
{
	int i;
	printf("Table Id  %x\n", pt.table_id);
	printf("section_syntax_indicator %d\n", pt.section_syntax_indicator);
	printf("private_indicator  %d\n", pt.private_indicator);
	printf("reserved  %d\n", pt.reserved);
	printf("section_length  %d\n", pt.section_length);
	printf("protocol_version  %d\n", pt.protocol_version);
	printf("encrypted_packet  %d\n", pt.encrypted_packet);
	printf("encryption_algorithm  %d\n", pt.encryption_algorithm);
	printf("pts_adjustment  %llu\n", pt.pts_adjustment);
	printf("cw_index  %d\n", pt.cw_index);
	printf("tier  %d\n", pt.tier);
	printf("splice_command_length  %d\n", pt.splice_command_length);
	switch (pt.splice_command_type)
	{
		case SPLICE_NULL:
			printf("Splice Command Type SPLICE NULL %d\n", pt.splice_command_type);
			break;
		case SPLICE_INSERT:
			printf("Splice Command Type SPLICE Insert %d\n", pt.splice_command_type);
			printf("splice_insert->splice_event_id  %d\n", pt.splice_insert->splice_event_id);
			printf("splice_insert->splice_event_cancel_indicator  %d\n", pt.splice_insert->splice_event_cancel_indicator);
			printf("splice_insert->reserved  %d\n", pt.splice_insert->reserved);
			printf("splice_insert->out_of_network_indicator  %d\n", pt.splice_insert->out_of_network_indicator);
			printf("splice_insert->program_splice_flag  %d\n", pt.splice_insert->program_splice_flag);
			printf("splice_insert->duration_flag  %d\n", pt.splice_insert->duration_flag);
			printf("splice_insert->splice_immediate_flag  %d\n", pt.splice_insert->splice_immediate_flag);
			printf("splice_insert->reserved1  %d\n", pt.splice_insert->reserved1);
			printf("splice_insert->single_splice_time.time_specified_flag  %d\n", pt.splice_insert->single_splice_time.time_specified_flag);
			printf("splice_insert->single_splice_time.reserved  %d\n", pt.splice_insert->single_splice_time.reserved);
			printf("splice_insert->single_splice_time.pts_time  %llu\n", pt.splice_insert->single_splice_time.pts_time);

			printf("splice_insert->break_duration.auto_return  %d\n", pt.splice_insert->break_duration.auto_return);
			printf("splice_insert->break_duration.reserved  %d\n", pt.splice_insert->break_duration.reserved);
			printf("splice_insert->break_duration.duration  %llu\n", pt.splice_insert->break_duration.duration);
			printf("splice_insert->unique_program_id  %d\n", pt.splice_insert->unique_program_id);
			printf("splice_insert->avail_num  %d\n", pt.splice_insert->avail_num);
			printf("splice_insert->avails_expected  %d\n", pt.splice_insert->avails_expected);
			break;
		
		case TIME_SIGNAL:
			printf("time_signal->splice_time.time_specified_flag %d\n", pt.time_signal->splice_time.time_specified_flag);
			printf("time_signal->splice_time.reserved %d\n", pt.time_signal->splice_time.reserved);
			printf("time_signal->splice_time.pts_time %llu\n", pt.time_signal->splice_time.pts_time);
			break;
		default:
			printf("Splice Command Type Not Implemented %d\n", pt.splice_command_type);
	}

	// splice_insert->break_duration

	printf("descriptor_loop_length  %d\n", pt.descriptor_loop_length);
	printf("descriptor Count %d\n", pt.descriptor_count);
	for (i=0;i<pt.descriptor_count;i++)
	{
		printf("splice descriptor %d\n", i);
		print_splice_descriptor(pt.splice_descriptor[i]);
	}
	printf("crc_32  %u\n", pt.crc_32);


	// print
	// splice_insert->single_splice_time

	// pt.splice_insert->splice_event_id


	// printf("Table Id  %d\n", pt.table_id);
}

void get_break_duration_payload(BreakDuration break_duration, uint8_t**payload)
{
	uint8_t* final_payload = (uint8_t*) *payload;
	uint8_t next_8_bits = (break_duration.auto_return << 7) & 0x80;
	next_8_bits |= ((break_duration.reserved << 1 ) & 0x7e);
	next_8_bits |= ((break_duration.duration >> 32) & 0x01);
	*final_payload++ = next_8_bits;
	*final_payload++ = ((break_duration.duration >> 24) & 0xff);
	*final_payload++ = ((break_duration.duration >> 16) & 0xff);
	*final_payload++ = ((break_duration.duration >> 8) & 0xff);
	*final_payload++ = (break_duration.duration & 0xff); 
	*payload = final_payload;

}

void get_splice_time_payload(SpliceTime splice_time, uint8_t**payload)
{
	uint8_t* final_payload = (uint8_t*) *payload;
	uint8_t next_8_bits = (splice_time.time_specified_flag << 7) & 0x80;
	if (splice_time.time_specified_flag == 0)
	{
		next_8_bits |= (splice_time.reserved & 0x7f);
		*final_payload++ = next_8_bits;
		*payload = final_payload;
		return;
	}
	next_8_bits |= ((splice_time.reserved << 1 ) & 0x7e);
	next_8_bits |= ((splice_time.pts_time >> 32) & 0x01);
	*final_payload++ = next_8_bits;
	*final_payload++ = ((splice_time.pts_time >> 24) & 0xff);
	*final_payload++ = ((splice_time.pts_time >> 16) & 0xff);
	*final_payload++ = ((splice_time.pts_time >> 8) & 0xff);
	*final_payload++ = (splice_time.pts_time & 0xff); 
	*payload = final_payload;

}

void get_time_signal_payload(TimeSignal *time_signal, uint8_t**payload)
{
	// printf("Inside Time Signal");
	get_splice_time_payload(time_signal->splice_time, payload);
}

void get_splice_insert_payload(SpliceInsert *splice_insert, uint8_t**payload)
{
	uint8_t next_8_bits;
	uint8_t* final_payload = (uint8_t*) *payload;

	// Do something;
	*final_payload++ = ((splice_insert->splice_event_id >> 24) & 0xff);
	*final_payload++ = ((splice_insert->splice_event_id >> 16) & 0xff);
	*final_payload++ = ((splice_insert->splice_event_id >> 8) & 0xff);
	*final_payload++ = (splice_insert->splice_event_id & 0xff); 

	next_8_bits = (splice_insert->splice_event_cancel_indicator << 7) & 0x80;
	next_8_bits |= (splice_insert->reserved & 0x7f);

	*final_payload++ = next_8_bits;

	if (splice_insert->splice_event_cancel_indicator == 0)
	{
		next_8_bits = (splice_insert->out_of_network_indicator << 7) & 0x80;
		next_8_bits |= ((splice_insert->program_splice_flag << 6) & 0x40);
		next_8_bits |= ((splice_insert->duration_flag << 5) & 0x20);
		next_8_bits |= ((splice_insert->splice_immediate_flag << 4) & 0x10);
		next_8_bits |= (splice_insert->reserved1 & 0x0f);
		*final_payload++ = next_8_bits;

		if (splice_insert->program_splice_flag == 1 && splice_insert->splice_immediate_flag==0)
		{
			get_splice_time_payload(splice_insert->single_splice_time, &final_payload);
		}
		if (splice_insert->program_splice_flag == 0)
		{
			printf("Not Implemented");
		}
		if (splice_insert->duration_flag==1)
		{
			get_break_duration_payload(splice_insert->break_duration, &final_payload);
		}
		*final_payload++ = (splice_insert->unique_program_id >> 8) & 0xff;
		*final_payload++ = splice_insert->unique_program_id  & 0xff;
		*final_payload++ = splice_insert->avail_num;
		*final_payload++ = splice_insert->avails_expected;
	}

	*payload = final_payload;

}

void update_splice_pts_time(SCTEPacket *pt, uint64_t pts_time)
{
	if (pt->splice_command_type == SPLICE_INSERT)
	{
		if (pt->splice_insert->splice_event_cancel_indicator != 0)
			return;
		if (!(pt->splice_insert->program_splice_flag == 1 && pt->splice_insert->splice_immediate_flag==0))
			return;
		if (pt->splice_insert->single_splice_time.time_specified_flag == 0)
			return;
		pt->splice_insert->single_splice_time.pts_time = pts_time;
		pt->modified = 1;
		return;
	}
	if (pt->splice_command_type == TIME_SIGNAL)
	{
		if (pt->time_signal->splice_time.time_specified_flag == 0)
			return;
		av_log(NULL, AV_LOG_DEBUG, "Updated PTS Time from %llu to %llu\n", pt->time_signal->splice_time.pts_time, pts_time);
		pt->time_signal->splice_time.pts_time = pts_time;
		pt->modified = 1;
	}
}

void get_segmentation_descriptor_payload(SegmentationDescriptor *segmentation_descriptor,uint8_t**payload)
{
	int i;
	uint8_t next_8_bits;
	uint8_t* final_payload = (uint8_t*) *payload;
	*final_payload++ = (segmentation_descriptor->segmentation_event_id >> 24) & 0xff;
	*final_payload++ = (segmentation_descriptor->segmentation_event_id >> 16) & 0xff;
	*final_payload++ = (segmentation_descriptor->segmentation_event_id >> 8) & 0xff;
	*final_payload++ = (segmentation_descriptor->segmentation_event_id & 0xff);
	next_8_bits = (segmentation_descriptor->segmentation_event_cancel_indicator << 7) & 0x80;
	next_8_bits |= (segmentation_descriptor->reserved & 0x7f);
	*final_payload++ = next_8_bits;
	if (segmentation_descriptor->segmentation_event_cancel_indicator == 0)
	{
		next_8_bits = (segmentation_descriptor->program_segmentation_flag  << 7) & 0x80;
		next_8_bits |= (segmentation_descriptor->segmentation_duration_flag  << 6) & 0x40;
		next_8_bits |= (segmentation_descriptor->delivery_not_restricted_flag  << 5) & 0x20;
		if (segmentation_descriptor->delivery_not_restricted_flag == 0)
		{
			next_8_bits |= (segmentation_descriptor->web_delivery_allowed_flag  << 4) & 0x10;
			next_8_bits |= (segmentation_descriptor->no_regional_blackout_flag  << 3) & 0x08;
			next_8_bits |= (segmentation_descriptor->archive_allowed_flag  << 2) & 0x04;
			next_8_bits |= (segmentation_descriptor->device_restrictions & 0x03);
		}
		else
		{
			next_8_bits |= (segmentation_descriptor->reserved1 & 0x1f);
		}
		*final_payload++ = next_8_bits;
		if (segmentation_descriptor->program_segmentation_flag == 0)
		{
			*final_payload++ = segmentation_descriptor->component_count;
			for (i=0; i<segmentation_descriptor->component_count; i++)
			{
				*final_payload++ = segmentation_descriptor->segmentation_components[i].component_tag;
				next_8_bits = (segmentation_descriptor->segmentation_components[i].reserved << 1) & 0xfe;
				next_8_bits |= (segmentation_descriptor->segmentation_components[i].pts_offset >> 32) & 0x01;
				*final_payload++ = next_8_bits;
				*final_payload++ = (segmentation_descriptor->segmentation_components[i].pts_offset >> 24) & 0xff;
				*final_payload++ = (segmentation_descriptor->segmentation_components[i].pts_offset >> 16) & 0xff;
				*final_payload++ = (segmentation_descriptor->segmentation_components[i].pts_offset >> 8) & 0xff;
				*final_payload++ = (segmentation_descriptor->segmentation_components[i].pts_offset & 0xff);
			}
		}
		if (segmentation_descriptor->segmentation_duration_flag == 1)
		{
			*final_payload++ = (segmentation_descriptor->segmentation_duration >> 32) & 0xff;
			*final_payload++ = (segmentation_descriptor->segmentation_duration >> 24) & 0xff;
			*final_payload++ = (segmentation_descriptor->segmentation_duration >> 16) & 0xff;
			*final_payload++ = (segmentation_descriptor->segmentation_duration >> 8) & 0xff;
			*final_payload++ = (segmentation_descriptor->segmentation_duration & 0xff);
		}
		*final_payload++ = segmentation_descriptor->segmentation_upid_type;
		*final_payload++ = segmentation_descriptor->segmentation_upid_length;
		for (i=0; i<segmentation_descriptor->segmentation_upid_length; i++)
			*final_payload++ = segmentation_descriptor->segmentation_upid_data[i];
		*final_payload++ = segmentation_descriptor->segmentation_type_id;	
		*final_payload++ = segmentation_descriptor->segment_num;
		*final_payload++ = segmentation_descriptor->segments_expected;
		if (segmentation_descriptor->segmentation_type_id == 0x34 || segmentation_descriptor->segmentation_type_id == 0x36)
		{
			*final_payload++ = segmentation_descriptor->sub_segment_num;
			*final_payload++ = segmentation_descriptor->sub_segments_expected;
		}	

	}
	*payload = final_payload;
}
void get_splice_descriptor_payload(SpliceDescriptor splice_descriptor, uint8_t**payload)
{
	int i;
	uint8_t next_8_bits;
	uint8_t* final_payload = (uint8_t*) *payload;
	*final_payload++ = splice_descriptor.splice_descriptor_tag & 0xff;
	*final_payload++ = splice_descriptor.descriptor_length & 0xff;
	*final_payload++ = (splice_descriptor.identifier >> 24) & 0xff;
	*final_payload++ = (splice_descriptor.identifier >> 16) & 0xff;
	*final_payload++ = (splice_descriptor.identifier >> 8) & 0xff;
	*final_payload++ = splice_descriptor.identifier & 0xff;
	switch (splice_descriptor.splice_descriptor_tag){
		case AVAIL_DESCRIPTOR:
			*final_payload++ = (splice_descriptor.avail_descriptor->provider_avail_id >> 24) & 0xff;
			*final_payload++ = (splice_descriptor.avail_descriptor->provider_avail_id >> 16) & 0xff;
			*final_payload++ = (splice_descriptor.avail_descriptor->provider_avail_id >> 8) & 0xff;
			*final_payload++ = splice_descriptor.avail_descriptor->provider_avail_id  & 0xff; 
			break;
		case DTMF_DESCRIPTOR:
			*final_payload++ = splice_descriptor.dtmf_descriptor->preroll;
			next_8_bits = (splice_descriptor.dtmf_descriptor->dtmf_count << 5 ) & 0xe0;
			next_8_bits |= (splice_descriptor.dtmf_descriptor->reserved & 0x1f);
			*final_payload++ = next_8_bits;
			for (i=0;i<splice_descriptor.dtmf_descriptor->dtmf_count; i++)
				*final_payload++ = splice_descriptor.dtmf_descriptor->DTMF_char[i];
			break;
		case SEGMENTATION_DESCRIPTOR:
			get_segmentation_descriptor_payload(splice_descriptor.segmentation_descriptor, &final_payload);
			break;
		case TIME_DESCRIPTOR:
			*final_payload++ = (splice_descriptor.time_descriptor->TAI_seconds >> 40) & 0xff;
			*final_payload++ = (splice_descriptor.time_descriptor->TAI_seconds >> 32) & 0xff;
			*final_payload++ = (splice_descriptor.time_descriptor->TAI_seconds >> 24) & 0xff;
			*final_payload++ = (splice_descriptor.time_descriptor->TAI_seconds >> 16) & 0xff;
			*final_payload++ = (splice_descriptor.time_descriptor->TAI_seconds >> 8) & 0xff;
			*final_payload++ = (splice_descriptor.time_descriptor->TAI_seconds & 0xff);
			*final_payload++ = (splice_descriptor.time_descriptor->TAI_ns >> 24) & 0xff;
			*final_payload++ =  (splice_descriptor.time_descriptor->TAI_ns >> 16) & 0xff;
			*final_payload++ =  (splice_descriptor.time_descriptor->TAI_ns >> 8) & 0xff;
			*final_payload++ =  (splice_descriptor.time_descriptor->TAI_ns & 0xff);
			*final_payload++ =  (splice_descriptor.time_descriptor->UTC_offset >> 8) & 0xff;
			*final_payload++ =  (splice_descriptor.time_descriptor->UTC_offset & 0xff);
			break;
		default:
			{
				int i;
				for (i=0;i<splice_descriptor.descriptor_length;i++)
					*final_payload++ = splice_descriptor.raw_bytes[i] & 0xff;
			}

	}
	
	*payload = final_payload;

}


void get_scte_payload(SCTEPacket pt, uint8_t** payload)
{
	if (pt.modified==0)
	{
		*payload = pt.init_payload;
		return;
	}
	int i=0;
	uint8_t next_8_bits;
	uint8_t* final_payload = (uint8_t*) malloc(pt.payload_size * sizeof(uint8_t));
	*payload = final_payload;
	*final_payload++  = pt.table_id;
	next_8_bits = (pt.section_syntax_indicator << 7) & 0x80;
	next_8_bits |= ((pt.private_indicator << 6) & 0x40);
	next_8_bits |= ((pt.reserved << 4) & 0x30);
	next_8_bits |= ((pt.section_length & 0x0f00) >> 8);
	*final_payload++ = next_8_bits;
	*final_payload++ = (pt.section_length & 0x00ff);
	*final_payload++ = pt.protocol_version;
	next_8_bits = (pt.encrypted_packet << 7) & 0x80;
	next_8_bits |= ((pt.encryption_algorithm << 1 ) & 0x7e);
	next_8_bits |= ((pt.pts_adjustment >> 32) & 0x01);
	*final_payload++ = next_8_bits;
	*final_payload++ = ((pt.pts_adjustment >> 24) & 0xff);
	*final_payload++ = ((pt.pts_adjustment >> 16) & 0xff);
	*final_payload++ = ((pt.pts_adjustment >> 8) & 0xff);
	*final_payload++ = (pt.pts_adjustment & 0xff); 
	*final_payload++ = pt.cw_index;
	*final_payload++ = ((pt.tier >> 4) & 0xff);
	next_8_bits = ((pt.tier & 0x0f) << 4);
	next_8_bits |= ((pt.splice_command_length>>8) & 0x0f);
	*final_payload++ = next_8_bits;

	*final_payload++ = (pt.splice_command_length & 0xff);
	*final_payload++ = pt.splice_command_type;

	switch (pt.splice_command_type) {
		case SPLICE_NULL:
			break;
		case SPLICE_INSERT:
			get_splice_insert_payload(pt.splice_insert, &final_payload);
			break;
		case TIME_SIGNAL:
			get_time_signal_payload(pt.time_signal, &final_payload);
			break;
		default:
			av_log(NULL, AV_LOG_ERROR,"Un Implemented SPLICE COMMAND TYPE");
			// Todo: Handle this
	}
	*final_payload++ = ((pt.descriptor_loop_length >> 8) & 0xff);
	*final_payload++ = (pt.descriptor_loop_length & 0xff);



	for (i=0; i< pt.descriptor_count; i++)
	{
		get_splice_descriptor_payload(pt.splice_descriptor[i], &final_payload);
	}


	pt.crc_32 = av_bswap32(av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1,(const uint8_t *) *payload, pt.payload_size-4));
	*final_payload++ = (pt.crc_32 >> 24) & 0xff;
	*final_payload++ = (pt.crc_32 >> 16) & 0xff;
	*final_payload++ = (pt.crc_32 >> 8) & 0xff;
	*final_payload++ = pt.crc_32 & 0xff;

	// for (int i=0; i<pt.payload_size-4;i++)
	// 	printf("%x ", (*payload)[i]);
	// printf("\n");
	// printf("AV_CRC_8_ATM Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_8_ATM), 0, *payload, pt.payload_size-4),pt.crc_32);
	// printf("AV_CRC_16_ANSI Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_16_ANSI), 0, *payload, pt.payload_size-4),pt.crc_32);
	// printf("AV_CRC_16_CCITT Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_16_CCITT), 0, *payload, pt.payload_size-4),pt.crc_32);
	// printf("AV_CRC_32_IEEE Calculated CRC %u .... Origina CRC %u\n",av_bswap32(av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1,(const uint8_t *) *payload, pt.payload_size-4)),pt.crc_32);
	// printf("AV_CRC_32_IEEE_LE Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_32_IEEE_LE), 0xFFFFFFFF, *payload, pt.payload_size-4)^0xFFFFFFFF,pt.crc_32);
	// printf("AV_CRC_16_ANSI_LE Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_16_ANSI_LE), 0, *payload, pt.payload_size-4),pt.crc_32);
	// printf("AV_CRC_24_IEEE Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_24_IEEE), 0, *payload, pt.payload_size-4),pt.crc_32);
	// printf("AV_CRC_8_EBU Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_8_EBU), 0, *payload, pt.payload_size-4),pt.crc_32);
	// printf("AV_CRC_MAX Calculated CRC %u .... Origina CRC %u\n",av_crc(av_crc_get_table(AV_CRC_MAX), 0, *payload, pt.payload_size-4),pt.crc_32); 
}
