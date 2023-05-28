#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "libavutil/crc.h"
#include <assert.h>
#include "libavutil/bswap.h"
#include "libavutil/log.h"


#define SCTE_UNIMPLEMENTED 404
enum SCTECommandType{
    SPLICE_NULL = 0x00,
    SPLICE_SCHEDULE = 0x04,
    SPLICE_INSERT = 0x05,
    TIME_SIGNAL = 0x06,
    BANDWIDTH_RESERVATION = 0x07,
    PRIVATE_COMMAND = 0xff
};

enum SpliceDescriptorTag {
	AVAIL_DESCRIPTOR = 0x00,
	DTMF_DESCRIPTOR = 0x01,
	SEGMENTATION_DESCRIPTOR = 0x02,
	TIME_DESCRIPTOR = 0x03
};

typedef struct AvailDescriptor {
	uint32_t provider_avail_id; // 32 bit
} AvailDescriptor;

typedef struct DTMFDescriptor {
	uint8_t preroll; // 8 bits
	uint8_t dtmf_count; // 3 bits 0x07
	uint8_t reserved; // 5 bits 0x1f
	uint8_t * DTMF_char;
}DTMFDescriptor;

typedef struct SegmentationComponent{
	uint8_t component_tag; // 8 bits 0xff
	uint8_t reserved; // 7 bits 0x7f
	uint64_t pts_offset; // 33 bits 0x0001ffffffff
}SegmentationComponent;

typedef struct SegmentationDescriptor {
	uint32_t segmentation_event_id; // 32 bits
	uint8_t segmentation_event_cancel_indicator; // 1 bit 0x01
	uint8_t reserved; // 7 bits 0x7f
	// if  (segmentation_event_cancel_indicator == 0)
	uint8_t program_segmentation_flag; // 1 bit 0x01
	uint8_t segmentation_duration_flag; // 1 bit 0x01
	uint8_t delivery_not_restricted_flag; // 1 bit 0x01
	// if delivery_not_restricted_flag == 0
	uint8_t web_delivery_allowed_flag; // 1 bit 0x01
	uint8_t no_regional_blackout_flag; // 1 bit 0x01
	uint8_t archive_allowed_flag; // 1 bit 0x01
	uint8_t device_restrictions; // 2 bit 0x03
	// else
	uint8_t reserved1; // 5 bits 0x1f

	// if program_segmentation_flag == 0
	uint8_t component_count; // 8 bits 0xff
	SegmentationComponent* segmentation_components;
	// if segmentation_duration_flag == 1
	uint64_t segmentation_duration; // 40 bits 0x00ffffffffff
	uint8_t segmentation_upid_type; // 8 bits 0xff
	uint8_t segmentation_upid_length; // 8 bits 0xff
	uint8_t * segmentation_upid_data; // will be segmentation_upid_length of bytes
	uint8_t segmentation_type_id; // 8 bits 0xff
	uint8_t segment_num; // 8 bits 0xff
	uint8_t segments_expected; // 8 bits 0xff
	// if (segementation_type_id == "0X34" || "0X36")
	uint8_t sub_segment_num; // 8 bits 0xff
	uint8_t sub_segments_expected; // 8 bits 0xff

} SegmentationDescriptor;

typedef struct TimeDescriptor {
	uint64_t TAI_seconds; // 48 bits
	uint32_t TAI_ns; // 32 bits
	uint16_t UTC_offset; // 16 bits
}TimeDescriptor;

typedef struct SpliceDescriptor {
	enum SpliceDescriptorTag splice_descriptor_tag; // 8 bits
	uint8_t descriptor_length; // 8 bits bytes length of descriptor
	uint32_t identifier; // 32 bits
	AvailDescriptor* avail_descriptor;
	DTMFDescriptor* dtmf_descriptor;
	SegmentationDescriptor* segmentation_descriptor;
	TimeDescriptor* time_descriptor;
	uint8_t *raw_bytes;

}SpliceDescriptor;

typedef struct SpliceTime {
	uint8_t time_specified_flag; // 1 bit 0x01

	// if time_specificed_flag == 1
	uint8_t reserved; // 6 bits (0x3f) if flag is true 7(0x7f) else
	uint64_t pts_time;  // 33 bits 0x0001ffffffff
}SpliceTime;


typedef struct BreakDuration {
	uint8_t auto_return; // 1 bit 0x01
	uint8_t reserved; // 6 bits (0x3f)
	uint64_t duration; // 33 bits 0x0001ffffffff
}BreakDuration;

typedef struct TimeSignal {
	SpliceTime splice_time;
}TimeSignal;

typedef struct SpliceInsert {
	// Splice Insert
	uint32_t splice_event_id; // 32 bits
	uint8_t splice_event_cancel_indicator; // 1 bit 0x01
	uint8_t reserved; // 7 bits 0x7f

	// if splice_event_cancel_indicator == '0'

	uint8_t out_of_network_indicator; // 1 bit 0x01
	uint8_t program_splice_flag; 	// 1 bit 0x01
	uint8_t duration_flag;			// 1 bit 0x01
	uint8_t splice_immediate_flag;	// 1 bit 0x01
	uint8_t reserved1; //4 bits 0x0f

	// If((program_splice_flag == ‘1’) && (splice_immediate_flag == ‘0’))

	SpliceTime single_splice_time;

	// if(program_splice_flag == ‘0’)
	uint8_t compontent_count; // 8 bits

	uint8_t *component_tag;
	// if(splice_immediate_flag == ‘0’)
	SpliceTime *multiple_splice_time;

	// if(duration_flag == ‘1’)
	BreakDuration break_duration;
	uint16_t unique_program_id; // 16 bits
	uint8_t avail_num; // 8 bits
	uint8_t avails_expected;  // 8 bits

}SpliceInsert;

typedef struct SCTEPacket {

	// Base SCTE Data
	uint8_t table_id; // 8 bits
	uint8_t section_syntax_indicator; // 1 bit 0x01
	uint8_t private_indicator; // 1 bit 0x01
	uint8_t reserved; // 2 bits 0x03
	uint16_t section_length; // 12 bits 0x0fff
	uint8_t protocol_version; // 8 bits
	uint8_t encrypted_packet; // 1 bit 0x01
	uint8_t encryption_algorithm; // 6 bits 0x2f
	uint64_t pts_adjustment; // 33 bits 0x0001ffffffff
	uint8_t cw_index; // 8 bits
	uint16_t tier; // 0x0fff 12 bits
	uint16_t splice_command_length; //0x0fff 12 bits
	enum SCTECommandType splice_command_type; // 8 bits

	SpliceInsert* splice_insert;
	TimeSignal* time_signal;

	uint16_t descriptor_loop_length; // 16 bit number of bytes of descriptor
	SpliceDescriptor * splice_descriptor;
	int descriptor_count; // Helper Variable
    // Todo: Alignmet_stuffing
    // Todo: E_CRC_32
	uint32_t crc_32;

	uint8_t *init_payload;
	int payload_size;

	int modified;

}SCTEPacket;


AvailDescriptor* init_avail_descriptor(uint8_t** payload_ptr, int *payload_size);

DTMFDescriptor* init_dtmf_descriptor(uint8_t** payload_ptr, int *payload_size);

SegmentationComponent init_segmentation_component(uint8_t** payload_ptr, int *payload_size);

SegmentationDescriptor* init_segmentation_descriptor(uint8_t** payload_ptr, int *payload_size);

TimeDescriptor* init_time_descriptor(uint8_t** payload_ptr, int* payload_size);

TimeSignal *init_time_signal(uint8_t** payload_ptr, int payload_size);

SpliceDescriptor init_splice_descriptor(uint8_t** payload_ptr, int* payload_size);

BreakDuration init_break_duration(uint8_t** payload_ptr, int payload_size);

SpliceTime init_splice_time(uint8_t** payload_ptr, int payload_size);

SpliceInsert *init_splice_insert(uint8_t** payload_ptr, int payload_size);

SCTEPacket init_scte_packet(uint8_t *payload, int payload_size);

void get_segmentation_descriptor_payload(SegmentationDescriptor *segmentation_descriptor,uint8_t**payload);

void get_splice_descriptor_payload(SpliceDescriptor splice_descriptor, uint8_t**payload);

void print_splice_descriptor(SpliceDescriptor splice_descriptor);

void print_scte_packet(SCTEPacket pt);

void get_break_duration_payload(BreakDuration break_duration, uint8_t**payload);

void get_splice_time_payload(SpliceTime splice_time, uint8_t**payload);

void get_time_signal_payload(TimeSignal *time_signal, uint8_t**payload);

void get_splice_insert_payload(SpliceInsert *splice_insert, uint8_t**payload);

void update_splice_pts_time(SCTEPacket *pt, uint64_t pts_time);

void get_scte_payload(SCTEPacket pt, uint8_t** payload);
