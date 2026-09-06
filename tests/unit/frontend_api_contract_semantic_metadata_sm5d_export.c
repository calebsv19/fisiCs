#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fisics_frontend.h"

static void init_extended(FisicsAnalysisResultEx* result) {
    memset(result, 0, sizeof(*result));
    result->abi_version = FISICS_ANALYSIS_RESULT_EX_ABI_VERSION;
    result->struct_size = sizeof(*result);
}

static int require_empty_lane(const FisicsAnalysisResultEx* result) {
    if (!result) return 1;
    if ((result->analysis.contract.capabilities &
         FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA) != 0 ||
        result->semantic_metadata_records != NULL ||
        result->semantic_metadata_record_count != 0 ||
        result->semantic_metadata_edges != NULL ||
        result->semantic_metadata_edge_count != 0) {
        fprintf(stderr, "semantic-metadata lane was not empty/unadvertised\n");
        return 1;
    }
    return 0;
}

int main(void) {
    static const char valid_source[] =
        "extern double temperature;\n"
        "[[fisics::id(\"method.read\")]]\n"
        "[[fisics::domain(\"thermal\")]]\n"
        "[[fisics::role(\"sensor.read\")]]\n"
        "[[fisics::produces(\"signal.temperature\")]]\n"
        "double read_temperature(void) { return temperature; }\n"
        "[[fisics::id(\"signal.temperature\")]]\n"
        "[[fisics::domain(\"thermal\")]]\n"
        "[[fisics::role(\"data.read\")]]\n"
        "double temperature = 21.0;\n"
        "[[fisics::domain(\"control\")]]\n"
        "[[fisics::role(\"control.decide\")]]\n"
        "[[fisics::consumes(\"signal.temperature\")]]\n"
        "[[fisics::produces(\"device.fan.target\")]]\n"
        "void control_without_id(void) {}\n";

    FisicsFrontendOptions opts = {0};
    opts.overlay_features = FISICS_OVERLAY_SEMANTIC_METADATA;

    FisicsAnalysisResultEx rejected;
    memset(&rejected, 0xA5, sizeof(rejected));
    rejected.abi_version = FISICS_ANALYSIS_RESULT_EX_ABI_VERSION + 1u;
    rejected.struct_size = sizeof(rejected);
    FisicsAnalysisResultEx rejected_before = rejected;
    if (fisics_analyze_buffer_ex("sm5_rejected.c", "int x;", 6, NULL,
                                 &rejected) ||
        memcmp(&rejected, &rejected_before, sizeof(rejected)) != 0) {
        fprintf(stderr, "unsupported ABI version modified caller storage\n");
        return 1;
    }

    memset(&rejected, 0x5A, sizeof(rejected));
    rejected.abi_version = FISICS_ANALYSIS_RESULT_EX_ABI_VERSION;
    rejected.struct_size = sizeof(rejected) - 1u;
    rejected_before = rejected;
    if (fisics_analyze_buffer_ex("sm5_rejected.c", "int x;", 6, NULL,
                                 &rejected) ||
        memcmp(&rejected, &rejected_before, sizeof(rejected)) != 0) {
        fprintf(stderr, "undersized ABI result modified caller storage\n");
        return 1;
    }

    FisicsAnalysisResultEx empty;
    init_extended(&empty);
    static const char plain_source[] = "int plain(void) { return 0; }\n";
    if (!fisics_analyze_buffer_ex("sm5_empty.c", plain_source,
                                  strlen(plain_source), &opts, &empty) ||
        require_empty_lane(&empty) != 0) {
        fisics_free_analysis_result_ex(&empty);
        return 1;
    }
    fisics_free_analysis_result_ex(&empty);

    static const char records_only_source[] =
        "[[fisics::id(\"identity.only\")]] int identity_only;\n";
    FisicsAnalysisResultEx records_only;
    init_extended(&records_only);
    if (!fisics_analyze_buffer_ex("sm5_records_only.c", records_only_source,
                                  strlen(records_only_source), &opts,
                                  &records_only) ||
        records_only.semantic_metadata_record_count != 1 ||
        records_only.semantic_metadata_edge_count != 0 ||
        !records_only.semantic_metadata_records[0].authored_id ||
        records_only.semantic_metadata_records[0].domain != NULL ||
        records_only.semantic_metadata_records[0].role != NULL) {
        fprintf(stderr, "records-only nullable projection did not match contract\n");
        fisics_free_analysis_result_ex(&records_only);
        return 1;
    }
    fisics_free_analysis_result_ex(&records_only);

    FisicsAnalysisResultEx result;
    init_extended(&result);
    if (!fisics_analyze_buffer_ex("sm5_valid.c", valid_source,
                                  strlen(valid_source), &opts, &result)) {
        fprintf(stderr, "valid extended metadata analysis failed\n");
        fisics_free_analysis_result_ex(&result);
        return 1;
    }
    if (result.analysis.contract.contract_major != 1 ||
        result.analysis.contract.contract_minor != 9 ||
        result.analysis.contract.contract_patch != 0 ||
        (result.analysis.contract.capabilities &
         FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA) == 0) {
        fprintf(stderr, "extended metadata contract/capability missing\n");
        fisics_free_analysis_result_ex(&result);
        return 1;
    }
    if (result.semantic_metadata_record_count != 3 ||
        result.semantic_metadata_edge_count != 3) {
        fprintf(stderr, "unexpected projection counts: records=%zu edges=%zu\n",
                result.semantic_metadata_record_count,
                result.semantic_metadata_edge_count);
        fisics_free_analysis_result_ex(&result);
        return 1;
    }

    const FisicsFrontendSemanticMetadataRecord* read =
        &result.semantic_metadata_records[0];
    const FisicsFrontendSemanticMetadataRecord* target =
        &result.semantic_metadata_records[1];
    const FisicsFrontendSemanticMetadataRecord* control =
        &result.semantic_metadata_records[2];
    if (!read->source_identity || strcmp(read->source_identity, "method.read") != 0 ||
        !read->authored_id || strcmp(read->authored_id, "method.read") != 0 ||
        !read->domain || strcmp(read->domain, "thermal") != 0 ||
        !read->role || strcmp(read->role, "sensor.read") != 0 ||
        read->symbol_kind != FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION ||
        !read->has_symbol_stable_id || read->symbol_stable_id == 0) {
        fprintf(stderr, "authored source record did not match contract\n");
        fisics_free_analysis_result_ex(&result);
        return 1;
    }
    if (!target->authored_id || strcmp(target->authored_id, "signal.temperature") != 0 ||
        target->symbol_kind != FISICS_SEMANTIC_METADATA_SYMBOL_FILE_VARIABLE ||
        !target->has_symbol_stable_id || target->symbol_stable_id == 0) {
        fprintf(stderr, "resolved target record did not match contract\n");
        fisics_free_analysis_result_ex(&result);
        return 1;
    }
    if (control->authored_id != NULL || !control->source_identity ||
        strncmp(control->source_identity, "symbol:function:", 16) != 0 ||
        strstr(control->source_identity, ":control_without_id") == NULL) {
        fprintf(stderr, "fallback source identity did not match contract\n");
        fisics_free_analysis_result_ex(&result);
        return 1;
    }

    const FisicsFrontendSemanticMetadataEdge* read_edge =
        &result.semantic_metadata_edges[0];
    const FisicsFrontendSemanticMetadataEdge* control_produces =
        &result.semantic_metadata_edges[1];
    const FisicsFrontendSemanticMetadataEdge* control_consumes =
        &result.semantic_metadata_edges[2];
    if (read_edge->source_record_index != 0 ||
        read_edge->kind != FISICS_SEMANTIC_METADATA_EDGE_PRODUCES ||
        strcmp(read_edge->target_id, "signal.temperature") != 0 ||
        read_edge->resolution != FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED ||
        !read_edge->has_target_record_index || read_edge->target_record_index != 1) {
        fprintf(stderr, "resolved edge did not match contract\n");
        fisics_free_analysis_result_ex(&result);
        return 1;
    }
    if (control_produces->source_record_index != 2 ||
        control_produces->kind != FISICS_SEMANTIC_METADATA_EDGE_PRODUCES ||
        control_produces->resolution != FISICS_SEMANTIC_METADATA_REFERENCE_EXTERNAL ||
        control_produces->has_target_record_index ||
        control_produces->target_record_index != 0 ||
        control_consumes->source_record_index != 2 ||
        control_consumes->kind != FISICS_SEMANTIC_METADATA_EDGE_CONSUMES ||
        control_consumes->resolution != FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED ||
        !control_consumes->has_target_record_index ||
        control_consumes->target_record_index != 1) {
        fprintf(stderr, "ordered control edges did not match contract\n");
        fisics_free_analysis_result_ex(&result);
        return 1;
    }

    /* Prove base-first and repeated destruction are safe. */
    fisics_free_analysis_result(&result.analysis);
    fisics_free_analysis_result_ex(&result);
    fisics_free_analysis_result_ex(&result);
    if (result.abi_version != FISICS_ANALYSIS_RESULT_EX_ABI_VERSION ||
        result.struct_size != sizeof(result) ||
        result.semantic_metadata_records != NULL ||
        result.semantic_metadata_record_count != 0 ||
        result.semantic_metadata_edges != NULL ||
        result.semantic_metadata_edge_count != 0) {
        return 1;
    }

    static const char invalid_source[] =
        "[[fisics::id(\"Bad.Identity\")]] int invalid(void);\n";
    FisicsAnalysisResultEx invalid;
    init_extended(&invalid);
    if (!fisics_analyze_buffer_ex("sm5_invalid.c", invalid_source,
                                  strlen(invalid_source), &opts, &invalid)) {
        fprintf(stderr, "invalid metadata did not return owned diagnostics\n");
        fisics_free_analysis_result_ex(&invalid);
        return 1;
    }
    if (!invalid.analysis.contract.partial || !invalid.analysis.contract.fatal ||
        invalid.analysis.diag_count == 0 || require_empty_lane(&invalid) != 0) {
        fprintf(stderr, "invalid metadata did not fail closed\n");
        fisics_free_analysis_result_ex(&invalid);
        return 1;
    }
    fisics_free_analysis_result_ex(&invalid);

    FisicsAnalysisResultEx allocation_failure;
    init_extended(&allocation_failure);
    if (setenv("FISICS_TEST_FAIL_SEMANTIC_METADATA_PROJECTION_ALLOC", "1", 1) != 0 ||
        fisics_analyze_buffer_ex("sm5_allocation_failure.c", valid_source,
                                 strlen(valid_source), &opts, &allocation_failure)) {
        unsetenv("FISICS_TEST_FAIL_SEMANTIC_METADATA_PROJECTION_ALLOC");
        fprintf(stderr, "projection allocation failure did not fail analysis\n");
        fisics_free_analysis_result_ex(&allocation_failure);
        return 1;
    }
    unsetenv("FISICS_TEST_FAIL_SEMANTIC_METADATA_PROJECTION_ALLOC");
    if (!allocation_failure.analysis.contract.partial ||
        !allocation_failure.analysis.contract.fatal ||
        require_empty_lane(&allocation_failure) != 0) {
        fprintf(stderr, "projection allocation failure did not clean up safely\n");
        fisics_free_analysis_result_ex(&allocation_failure);
        return 1;
    }
    fisics_free_analysis_result_ex(&allocation_failure);

    FisicsAnalysisResult legacy = {0};
    if (!fisics_analyze_buffer("sm5_legacy_metadata.c", valid_source,
                               strlen(valid_source), &opts, &legacy) ||
        (legacy.contract.capabilities &
         FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA) != 0) {
        fprintf(stderr, "legacy API advertised extended metadata\n");
        fisics_free_analysis_result(&legacy);
        return 1;
    }
    fisics_free_analysis_result(&legacy);
    return 0;
}
