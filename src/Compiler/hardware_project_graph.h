// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Compiler/hardware_project_manifest.h"

typedef struct FisicsHardwareStringList {
  char **items;
  size_t count;
} FisicsHardwareStringList;

typedef struct FisicsHardwareRef {
  char *id;
  char *revision;
} FisicsHardwareRef;

typedef struct FisicsHardwareReceipt {
  char *path;
  uint64_t hash;
  size_t length;
  char *catalog_id;
  char *authority_kind;
  char *authority_identity;
} FisicsHardwareReceipt;

typedef struct FisicsHardwareAllocationSpace {
  char *space_id;
  char *value_kind;
  char *minimum;
  char *maximum;
  bool claim_required;
  char *collision_policy;
} FisicsHardwareAllocationSpace;

typedef struct FisicsHardwareBusProfile {
  char *id;
  char *revision;
  char *transport_kind;
  char *controller_capability;
  char *participant_capability;
  FisicsHardwareAllocationSpace *spaces;
  size_t space_count;
  FisicsHardwareStringList origins;
} FisicsHardwareBusProfile;

typedef struct FisicsHardwareEndpoint {
  char *id;
  FisicsHardwareStringList capabilities;
  char *allocation_policy;
} FisicsHardwareEndpoint;

typedef struct FisicsHardwareBoardProfile {
  char *id;
  char *revision;
  FisicsHardwareStringList capabilities;
  FisicsHardwareEndpoint *endpoints;
  size_t endpoint_count;
  FisicsHardwareStringList origins;
} FisicsHardwareBoardProfile;

typedef struct FisicsHardwareAttachmentPort {
  char *id;
  char *connection_kind;
  char *required_capability;
  char *required_transport_kind;
  bool required;
} FisicsHardwareAttachmentPort;

typedef struct FisicsHardwarePhysicalRequirement {
  char *mode;
  char *quantity_kind;
  char *dimension_system;
  int8_t dimension[8];
  bool has_dimension;
  char *value_form;
  char *reference_kind;
} FisicsHardwarePhysicalRequirement;

typedef struct FisicsHardwarePhysicalPort {
  char *id;
  char *direction;
  bool required;
  FisicsHardwarePhysicalRequirement requirement;
} FisicsHardwarePhysicalPort;

typedef struct FisicsHardwareDeviceModel {
  char *id;
  char *revision;
  FisicsHardwareStringList capabilities;
  char *driver_policy;
  FisicsHardwareAttachmentPort *attachment_ports;
  size_t attachment_port_count;
  FisicsHardwarePhysicalPort *physical_ports;
  size_t physical_port_count;
  FisicsHardwareStringList origins;
} FisicsHardwareDeviceModel;

typedef struct FisicsHardwareDriverContract {
  char *id;
  char *revision;
  FisicsHardwareRef *supported_models;
  size_t supported_model_count;
  FisicsHardwareStringList required_transports;
  FisicsHardwareStringList origins;
} FisicsHardwareDriverContract;

typedef struct FisicsHardwareNode {
  char *id;
  FisicsHardwareRef board_profile;
  size_t board_profile_index;
} FisicsHardwareNode;

typedef struct FisicsHardwareBus {
  char *id;
  char *owner_node_id;
  char *scope;
  FisicsHardwareRef bus_profile;
  char *controller_endpoint_id;
  size_t owner_node_index;
  size_t bus_profile_index;
  size_t controller_endpoint_index;
} FisicsHardwareBus;

typedef struct FisicsHardwareDevice {
  char *id;
  char *owner_node_id;
  FisicsHardwareRef model;
  FisicsHardwareRef driver;
  bool has_driver;
  size_t owner_node_index;
  size_t model_index;
  size_t driver_index;
} FisicsHardwareDevice;

typedef struct FisicsHardwareAllocationClaim {
  char *space_id;
  char *value_uint64;
  char *value_id;
  size_t space_index;
} FisicsHardwareAllocationClaim;

typedef struct FisicsHardwareConnection {
  char *id;
  char *device_id;
  char *attachment_port_id;
  char *target_kind;
  char *bus_id;
  char *board_endpoint_id;
  FisicsHardwareAllocationClaim *claims;
  size_t claim_count;
  size_t device_index;
  size_t attachment_port_index;
  size_t target_index;
} FisicsHardwareConnection;

typedef struct FisicsHardwareSignalBinding {
  char *id;
  char *device_id;
  char *physical_port_id;
  char *physical_contract_id;
  size_t device_index;
  size_t physical_port_index;
  size_t physical_contract_index;
  char *physical_evidence_state;
  char *compatibility_mode;
} FisicsHardwareSignalBinding;

typedef struct FisicsHardwareUnboundPort {
  char *device_id;
  char *port_kind;
  char *port_id;
} FisicsHardwareUnboundPort;

typedef struct FisicsHardwareProjectGraph {
  char *project_id;
  char *manifest_path;
  char *physical_graph_path;
  uint64_t physical_graph_hash;
  size_t physical_graph_length;
  char *unit_registry_fingerprint;
  FisicsHardwareStringList physical_source_paths;
  FisicsHardwareReceipt *receipts;
  size_t receipt_count;
  FisicsHardwareBusProfile *bus_profiles;
  size_t bus_profile_count;
  FisicsHardwareBoardProfile *board_profiles;
  size_t board_profile_count;
  FisicsHardwareDeviceModel *device_models;
  size_t device_model_count;
  FisicsHardwareDriverContract *driver_contracts;
  size_t driver_contract_count;
  FisicsHardwareNode *nodes;
  size_t node_count;
  FisicsHardwareBus *buses;
  size_t bus_count;
  FisicsHardwareDevice *devices;
  size_t device_count;
  FisicsHardwareConnection *connections;
  size_t connection_count;
  FisicsHardwareSignalBinding *signal_bindings;
  size_t signal_binding_count;
  FisicsHardwareUnboundPort *unbound_ports;
  size_t unbound_port_count;
} FisicsHardwareProjectGraph;

bool fisics_hardware_project_graph_build(
    const FisicsHardwareProjectManifest *manifest,
    FisicsHardwareProjectGraph *out,
    FisicsSemanticProjectDiagnostic *diagnostic);

void fisics_hardware_project_graph_free(FisicsHardwareProjectGraph *graph);
