                                GBLA THUNK_RETURN_ADDRESS_OFFSET
THUNK_RETURN_ADDRESS_OFFSET     SETA 0x38
                                GBLA PRESERVED_RETURN_ADDRESS_OFFSET
PRESERVED_RETURN_ADDRESS_OFFSET SETA 0x3C
                                GBLA PRESERVED_R4_OFFSET
PRESERVED_R4_OFFSET             SETA 0x40
                                GBLA PRESERVED_R5_OFFSET
PRESERVED_R5_OFFSET             SETA 0x44
                                GBLA PRESERVED_R6_OFFSET
PRESERVED_R6_OFFSET             SETA 0x48
                                GBLA PRESERVED_R7_OFFSET
PRESERVED_R7_OFFSET             SETA 0x4C
                                GBLA PRESERVED_R8_OFFSET
PRESERVED_R8_OFFSET             SETA 0x50
                                GBLA PRESERVED_R9_OFFSET
PRESERVED_R9_OFFSET             SETA 0x54
                                GBLA PRESERVED_R10_OFFSET
PRESERVED_R10_OFFSET            SETA 0x58
                                GBLA PRESERVED_R11_OFFSET
PRESERVED_R11_OFFSET            SETA 0x5C
                                GBLA REGISTER_FILE_OFFSET
REGISTER_FILE_OFFSET            SETA 0x60
                                GBLA FIRST_STACK_ARGUMENT
FIRST_STACK_ARGUMENT            SETA 0x68

    AREA Trampoline, CODE

    EXPORT ctiTrampoline
    EXPORT ctiTrampolineEnd
    EXPORT ctiVMThrowTrampoline
    EXPORT ctiOpThrowNotCaught

ctiTrampoline PROC
    sub sp, sp, # FIRST_STACK_ARGUMENT
    str lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    str r4, [sp, # PRESERVED_R4_OFFSET ]
    str r5, [sp, # PRESERVED_R5_OFFSET ]
    str r6, [sp, # PRESERVED_R6_OFFSET ]
    str r7, [sp, # PRESERVED_R7_OFFSET ]
    str r8, [sp, # PRESERVED_R8_OFFSET ]
    str r9, [sp, # PRESERVED_R9_OFFSET ]
    str r10, [sp, # PRESERVED_R10_OFFSET ]
    str r11, [sp, # PRESERVED_R11_OFFSET ]
    str r1, [sp, # REGISTER_FILE_OFFSET ]
    mov r5, r2
    blx r0
    ldr r11, [sp, # PRESERVED_R11_OFFSET ]
    ldr r10, [sp, # PRESERVED_R10_OFFSET ]
    ldr r9, [sp, # PRESERVED_R9_OFFSET ]
    ldr r8, [sp, # PRESERVED_R8_OFFSET ]
    ldr r7, [sp, # PRESERVED_R7_OFFSET ]
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r5, [sp, # PRESERVED_R5_OFFSET ]
    ldr r4, [sp, # PRESERVED_R4_OFFSET ]
    ldr lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    add sp, sp, # FIRST_STACK_ARGUMENT
    bx lr
ctiTrampolineEnd
ctiTrampoline ENDP

ctiVMThrowTrampoline PROC
    mov r0, sp
    bl cti_vm_throw
    ldr r11, [sp, # PRESERVED_R11_OFFSET ]
    ldr r10, [sp, # PRESERVED_R10_OFFSET ]
    ldr r9, [sp, # PRESERVED_R9_OFFSET ]
    ldr r8, [sp, # PRESERVED_R8_OFFSET ]
    ldr r7, [sp, # PRESERVED_R7_OFFSET ]
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r5, [sp, # PRESERVED_R5_OFFSET ]
    ldr r4, [sp, # PRESERVED_R4_OFFSET ]
    ldr lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    add sp, sp, # FIRST_STACK_ARGUMENT
    bx lr
ctiVMThrowTrampoline ENDP

ctiOpThrowNotCaught PROC
    ldr r11, [sp, # PRESERVED_R11_OFFSET ]
    ldr r10, [sp, # PRESERVED_R10_OFFSET ]
    ldr r9, [sp, # PRESERVED_R9_OFFSET ]
    ldr r8, [sp, # PRESERVED_R8_OFFSET ]
    ldr r7, [sp, # PRESERVED_R7_OFFSET ]
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r5, [sp, # PRESERVED_R5_OFFSET ]
    ldr r4, [sp, # PRESERVED_R4_OFFSET ]
    ldr lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    add sp, sp, # FIRST_STACK_ARGUMENT
    bx lr
ctiOpThrowNotCaught ENDP

    EXPORT cti_op_create_this
    IMPORT JITStubThunked_op_create_this
cti_op_create_this PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_create_this
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_create_this ENDP

    EXPORT cti_op_convert_this
    IMPORT JITStubThunked_op_convert_this
cti_op_convert_this PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_convert_this
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_convert_this ENDP

    EXPORT cti_op_add
    IMPORT JITStubThunked_op_add
cti_op_add PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_add
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_add ENDP

    EXPORT cti_op_inc
    IMPORT JITStubThunked_op_inc
cti_op_inc PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_inc
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_inc ENDP

    EXPORT cti_handle_watchdog_timer
    IMPORT JITStubThunked_handle_watchdog_timer
cti_handle_watchdog_timer PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_handle_watchdog_timer
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_handle_watchdog_timer ENDP

    EXPORT cti_stack_check
    IMPORT JITStubThunked_stack_check
cti_stack_check PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_stack_check
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_stack_check ENDP

    EXPORT cti_op_new_object
    IMPORT JITStubThunked_op_new_object
cti_op_new_object PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_object
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_new_object ENDP

    EXPORT cti_op_put_by_id_generic
    IMPORT JITStubThunked_op_put_by_id_generic
cti_op_put_by_id_generic PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_id_generic ENDP

    EXPORT cti_op_put_by_id_direct_generic
    IMPORT JITStubThunked_op_put_by_id_direct_generic
cti_op_put_by_id_direct_generic PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_direct_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_id_direct_generic ENDP

    EXPORT cti_op_get_by_id_generic
    IMPORT JITStubThunked_op_get_by_id_generic
cti_op_get_by_id_generic PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_generic ENDP

    EXPORT cti_op_put_by_id
    IMPORT JITStubThunked_op_put_by_id
cti_op_put_by_id PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_id ENDP

    EXPORT cti_op_put_by_id_direct
    IMPORT JITStubThunked_op_put_by_id_direct
cti_op_put_by_id_direct PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_direct
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_id_direct ENDP

    EXPORT cti_op_put_by_id_fail
    IMPORT JITStubThunked_op_put_by_id_fail
cti_op_put_by_id_fail PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_id_fail ENDP

    EXPORT cti_op_put_by_id_direct_fail
    IMPORT JITStubThunked_op_put_by_id_direct_fail
cti_op_put_by_id_direct_fail PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_direct_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_id_direct_fail ENDP

    EXPORT cti_op_put_by_id_transition_realloc
    IMPORT JITStubThunked_op_put_by_id_transition_realloc
cti_op_put_by_id_transition_realloc PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_id_transition_realloc
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_id_transition_realloc ENDP

    EXPORT cti_op_get_by_id
    IMPORT JITStubThunked_op_get_by_id
cti_op_get_by_id PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id ENDP

    EXPORT cti_op_get_by_id_self_fail
    IMPORT JITStubThunked_op_get_by_id_self_fail
cti_op_get_by_id_self_fail PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_self_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_self_fail ENDP

    EXPORT cti_op_get_by_id_getter_stub
    IMPORT JITStubThunked_op_get_by_id_getter_stub
cti_op_get_by_id_getter_stub PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_getter_stub
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_getter_stub ENDP

    EXPORT cti_op_get_by_id_custom_stub
    IMPORT JITStubThunked_op_get_by_id_custom_stub
cti_op_get_by_id_custom_stub PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_custom_stub
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_custom_stub ENDP

    EXPORT cti_op_get_by_id_proto_list
    IMPORT JITStubThunked_op_get_by_id_proto_list
cti_op_get_by_id_proto_list PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_proto_list
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_proto_list ENDP

    EXPORT cti_op_get_by_id_proto_list_full
    IMPORT JITStubThunked_op_get_by_id_proto_list_full
cti_op_get_by_id_proto_list_full PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_proto_list_full
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_proto_list_full ENDP

    EXPORT cti_op_get_by_id_proto_fail
    IMPORT JITStubThunked_op_get_by_id_proto_fail
cti_op_get_by_id_proto_fail PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_proto_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_proto_fail ENDP

    EXPORT cti_op_get_by_id_array_fail
    IMPORT JITStubThunked_op_get_by_id_array_fail
cti_op_get_by_id_array_fail PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_array_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_array_fail ENDP

    EXPORT cti_op_get_by_id_string_fail
    IMPORT JITStubThunked_op_get_by_id_string_fail
cti_op_get_by_id_string_fail PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_id_string_fail
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_id_string_fail ENDP

    EXPORT cti_op_check_has_instance
    IMPORT JITStubThunked_op_check_has_instance
cti_op_check_has_instance PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_check_has_instance
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_check_has_instance ENDP

    EXPORT cti_op_instanceof
    IMPORT JITStubThunked_op_instanceof
cti_op_instanceof PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_instanceof
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_instanceof ENDP

    EXPORT cti_op_del_by_id
    IMPORT JITStubThunked_op_del_by_id
cti_op_del_by_id PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_del_by_id
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_del_by_id ENDP

    EXPORT cti_op_mul
    IMPORT JITStubThunked_op_mul
cti_op_mul PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_mul
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_mul ENDP

    EXPORT cti_op_new_func
    IMPORT JITStubThunked_op_new_func
cti_op_new_func PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_func
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_new_func ENDP

    EXPORT cti_op_call_jitCompile
    IMPORT JITStubThunked_op_call_jitCompile
cti_op_call_jitCompile PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_jitCompile
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_call_jitCompile ENDP

    EXPORT cti_op_construct_jitCompile
    IMPORT JITStubThunked_op_construct_jitCompile
cti_op_construct_jitCompile PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_construct_jitCompile
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_construct_jitCompile ENDP

    EXPORT cti_op_call_arityCheck
    IMPORT JITStubThunked_op_call_arityCheck
cti_op_call_arityCheck PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_arityCheck
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_call_arityCheck ENDP

    EXPORT cti_op_construct_arityCheck
    IMPORT JITStubThunked_op_construct_arityCheck
cti_op_construct_arityCheck PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_construct_arityCheck
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_construct_arityCheck ENDP

    EXPORT cti_vm_lazyLinkCall
    IMPORT JITStubThunked_vm_lazyLinkCall
cti_vm_lazyLinkCall PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_vm_lazyLinkCall
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_vm_lazyLinkCall ENDP

    EXPORT cti_vm_lazyLinkClosureCall
    IMPORT JITStubThunked_vm_lazyLinkClosureCall
cti_vm_lazyLinkClosureCall PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_vm_lazyLinkClosureCall
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_vm_lazyLinkClosureCall ENDP

    EXPORT cti_vm_lazyLinkConstruct
    IMPORT JITStubThunked_vm_lazyLinkConstruct
cti_vm_lazyLinkConstruct PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_vm_lazyLinkConstruct
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_vm_lazyLinkConstruct ENDP

    EXPORT cti_op_push_activation
    IMPORT JITStubThunked_op_push_activation
cti_op_push_activation PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_push_activation
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_push_activation ENDP

    EXPORT cti_op_call_NotJSFunction
    IMPORT JITStubThunked_op_call_NotJSFunction
cti_op_call_NotJSFunction PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_NotJSFunction
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_call_NotJSFunction ENDP

    EXPORT cti_op_create_arguments
    IMPORT JITStubThunked_op_create_arguments
cti_op_create_arguments PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_create_arguments
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_create_arguments ENDP

    EXPORT cti_op_tear_off_activation
    IMPORT JITStubThunked_op_tear_off_activation
cti_op_tear_off_activation PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_tear_off_activation
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_tear_off_activation ENDP

    EXPORT cti_op_tear_off_arguments
    IMPORT JITStubThunked_op_tear_off_arguments
cti_op_tear_off_arguments PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_tear_off_arguments
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_tear_off_arguments ENDP

    EXPORT cti_op_profile_will_call
    IMPORT JITStubThunked_op_profile_will_call
cti_op_profile_will_call PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_profile_will_call
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_profile_will_call ENDP

    EXPORT cti_op_profile_did_call
    IMPORT JITStubThunked_op_profile_did_call
cti_op_profile_did_call PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_profile_did_call
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_profile_did_call ENDP

    EXPORT cti_op_new_array
    IMPORT JITStubThunked_op_new_array
cti_op_new_array PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_array
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_new_array ENDP

    EXPORT cti_op_new_array_with_size
    IMPORT JITStubThunked_op_new_array_with_size
cti_op_new_array_with_size PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_array_with_size
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_new_array_with_size ENDP

    EXPORT cti_op_new_array_buffer
    IMPORT JITStubThunked_op_new_array_buffer
cti_op_new_array_buffer PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_array_buffer
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_new_array_buffer ENDP

    EXPORT cti_op_init_global_const_check
    IMPORT JITStubThunked_op_init_global_const_check
cti_op_init_global_const_check PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_init_global_const_check
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_init_global_const_check ENDP

    EXPORT cti_op_resolve
    IMPORT JITStubThunked_op_resolve
cti_op_resolve PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_resolve ENDP

    EXPORT cti_op_put_to_base
    IMPORT JITStubThunked_op_put_to_base
cti_op_put_to_base PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_to_base
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_to_base ENDP

    EXPORT cti_op_construct_NotJSConstruct
    IMPORT JITStubThunked_op_construct_NotJSConstruct
cti_op_construct_NotJSConstruct PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_construct_NotJSConstruct
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_construct_NotJSConstruct ENDP

    EXPORT cti_op_get_by_val
    IMPORT JITStubThunked_op_get_by_val
cti_op_get_by_val PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_val
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_val ENDP

    EXPORT cti_op_get_by_val_generic
    IMPORT JITStubThunked_op_get_by_val_generic
cti_op_get_by_val_generic PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_val_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_val_generic ENDP

    EXPORT cti_op_get_by_val_string
    IMPORT JITStubThunked_op_get_by_val_string
cti_op_get_by_val_string PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_by_val_string
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_by_val_string ENDP

    EXPORT cti_op_sub
    IMPORT JITStubThunked_op_sub
cti_op_sub PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_sub
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_sub ENDP

    EXPORT cti_op_put_by_val
    IMPORT JITStubThunked_op_put_by_val
cti_op_put_by_val PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_val
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_val ENDP

    EXPORT cti_op_put_by_val_generic
    IMPORT JITStubThunked_op_put_by_val_generic
cti_op_put_by_val_generic PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_val_generic
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_val_generic ENDP

    EXPORT cti_op_less
    IMPORT JITStubThunked_op_less
cti_op_less PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_less
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_less ENDP

    EXPORT cti_op_lesseq
    IMPORT JITStubThunked_op_lesseq
cti_op_lesseq PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_lesseq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_lesseq ENDP

    EXPORT cti_op_greater
    IMPORT JITStubThunked_op_greater
cti_op_greater PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_greater
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_greater ENDP

    EXPORT cti_op_greatereq
    IMPORT JITStubThunked_op_greatereq
cti_op_greatereq PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_greatereq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_greatereq ENDP

    EXPORT cti_op_load_varargs
    IMPORT JITStubThunked_op_load_varargs
cti_op_load_varargs PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_load_varargs
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_load_varargs ENDP

    EXPORT cti_op_negate
    IMPORT JITStubThunked_op_negate
cti_op_negate PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_negate
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_negate ENDP

    EXPORT cti_op_resolve_base
    IMPORT JITStubThunked_op_resolve_base
cti_op_resolve_base PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_base
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_resolve_base ENDP

    EXPORT cti_op_resolve_base_strict_put
    IMPORT JITStubThunked_op_resolve_base_strict_put
cti_op_resolve_base_strict_put PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_base_strict_put
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_resolve_base_strict_put ENDP

    EXPORT cti_op_div
    IMPORT JITStubThunked_op_div
cti_op_div PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_div
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_div ENDP

    EXPORT cti_op_dec
    IMPORT JITStubThunked_op_dec
cti_op_dec PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_dec
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_dec ENDP

    EXPORT cti_op_jless
    IMPORT JITStubThunked_op_jless
cti_op_jless PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jless
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_jless ENDP

    EXPORT cti_op_jlesseq
    IMPORT JITStubThunked_op_jlesseq
cti_op_jlesseq PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jlesseq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_jlesseq ENDP

    EXPORT cti_op_jgreater
    IMPORT JITStubThunked_op_jgreater
cti_op_jgreater PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jgreater
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_jgreater ENDP

    EXPORT cti_op_jgreatereq
    IMPORT JITStubThunked_op_jgreatereq
cti_op_jgreatereq PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jgreatereq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_jgreatereq ENDP

    EXPORT cti_op_not
    IMPORT JITStubThunked_op_not
cti_op_not PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_not
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_not ENDP

    EXPORT cti_op_jtrue
    IMPORT JITStubThunked_op_jtrue
cti_op_jtrue PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_jtrue
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_jtrue ENDP

    EXPORT cti_op_eq
    IMPORT JITStubThunked_op_eq
cti_op_eq PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_eq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_eq ENDP

    EXPORT cti_op_eq_strings
    IMPORT JITStubThunked_op_eq_strings
cti_op_eq_strings PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_eq_strings
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_eq_strings ENDP

    EXPORT cti_op_lshift
    IMPORT JITStubThunked_op_lshift
cti_op_lshift PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_lshift
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_lshift ENDP

    EXPORT cti_op_bitand
    IMPORT JITStubThunked_op_bitand
cti_op_bitand PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_bitand
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_bitand ENDP

    EXPORT cti_op_rshift
    IMPORT JITStubThunked_op_rshift
cti_op_rshift PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_rshift
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_rshift ENDP

    EXPORT cti_op_resolve_with_base
    IMPORT JITStubThunked_op_resolve_with_base
cti_op_resolve_with_base PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_with_base
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_resolve_with_base ENDP

    EXPORT cti_op_resolve_with_this
    IMPORT JITStubThunked_op_resolve_with_this
cti_op_resolve_with_this PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_resolve_with_this
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_resolve_with_this ENDP

    EXPORT cti_op_new_func_exp
    IMPORT JITStubThunked_op_new_func_exp
cti_op_new_func_exp PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_func_exp
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_new_func_exp ENDP

    EXPORT cti_op_mod
    IMPORT JITStubThunked_op_mod
cti_op_mod PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_mod
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_mod ENDP

    EXPORT cti_op_urshift
    IMPORT JITStubThunked_op_urshift
cti_op_urshift PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_urshift
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_urshift ENDP

    EXPORT cti_op_bitxor
    IMPORT JITStubThunked_op_bitxor
cti_op_bitxor PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_bitxor
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_bitxor ENDP

    EXPORT cti_op_new_regexp
    IMPORT JITStubThunked_op_new_regexp
cti_op_new_regexp PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_new_regexp
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_new_regexp ENDP

    EXPORT cti_op_bitor
    IMPORT JITStubThunked_op_bitor
cti_op_bitor PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_bitor
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_bitor ENDP

    EXPORT cti_op_call_eval
    IMPORT JITStubThunked_op_call_eval
cti_op_call_eval PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_call_eval
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_call_eval ENDP

    EXPORT cti_op_throw
    IMPORT JITStubThunked_op_throw
cti_op_throw PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_throw
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_throw ENDP

    EXPORT cti_op_get_pnames
    IMPORT JITStubThunked_op_get_pnames
cti_op_get_pnames PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_get_pnames
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_get_pnames ENDP

    EXPORT cti_has_property
    IMPORT JITStubThunked_has_property
cti_has_property PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_has_property
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_has_property ENDP

    EXPORT cti_op_push_with_scope
    IMPORT JITStubThunked_op_push_with_scope
cti_op_push_with_scope PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_push_with_scope
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_push_with_scope ENDP

    EXPORT cti_op_pop_scope
    IMPORT JITStubThunked_op_pop_scope
cti_op_pop_scope PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_pop_scope
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_pop_scope ENDP

    EXPORT cti_op_typeof
    IMPORT JITStubThunked_op_typeof
cti_op_typeof PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_typeof
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_typeof ENDP

    EXPORT cti_op_is_object
    IMPORT JITStubThunked_op_is_object
cti_op_is_object PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_object
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_is_object ENDP

    EXPORT cti_op_is_function
    IMPORT JITStubThunked_op_is_function
cti_op_is_function PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_is_function
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_is_function ENDP

    EXPORT cti_op_stricteq
    IMPORT JITStubThunked_op_stricteq
cti_op_stricteq PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_stricteq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_stricteq ENDP

    EXPORT cti_op_to_primitive
    IMPORT JITStubThunked_op_to_primitive
cti_op_to_primitive PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_to_primitive
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_to_primitive ENDP

    EXPORT cti_op_strcat
    IMPORT JITStubThunked_op_strcat
cti_op_strcat PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_strcat
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_strcat ENDP

    EXPORT cti_op_nstricteq
    IMPORT JITStubThunked_op_nstricteq
cti_op_nstricteq PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_nstricteq
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_nstricteq ENDP

    EXPORT cti_op_to_number
    IMPORT JITStubThunked_op_to_number
cti_op_to_number PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_to_number
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_to_number ENDP

    EXPORT cti_op_in
    IMPORT JITStubThunked_op_in
cti_op_in PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_in
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_in ENDP

    EXPORT cti_op_push_name_scope
    IMPORT JITStubThunked_op_push_name_scope
cti_op_push_name_scope PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_push_name_scope
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_push_name_scope ENDP

    EXPORT cti_op_put_by_index
    IMPORT JITStubThunked_op_put_by_index
cti_op_put_by_index PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_by_index
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_by_index ENDP

    EXPORT cti_op_switch_imm
    IMPORT JITStubThunked_op_switch_imm
cti_op_switch_imm PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_switch_imm
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_switch_imm ENDP

    EXPORT cti_op_switch_char
    IMPORT JITStubThunked_op_switch_char
cti_op_switch_char PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_switch_char
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_switch_char ENDP

    EXPORT cti_op_switch_string
    IMPORT JITStubThunked_op_switch_string
cti_op_switch_string PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_switch_string
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_switch_string ENDP

    EXPORT cti_op_del_by_val
    IMPORT JITStubThunked_op_del_by_val
cti_op_del_by_val PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_del_by_val
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_del_by_val ENDP

    EXPORT cti_op_put_getter_setter
    IMPORT JITStubThunked_op_put_getter_setter
cti_op_put_getter_setter PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_put_getter_setter
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_put_getter_setter ENDP

    EXPORT cti_op_throw_static_error
    IMPORT JITStubThunked_op_throw_static_error
cti_op_throw_static_error PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_throw_static_error
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_throw_static_error ENDP

    EXPORT cti_op_debug
    IMPORT JITStubThunked_op_debug
cti_op_debug PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_op_debug
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_op_debug ENDP

    EXPORT cti_vm_throw
    IMPORT JITStubThunked_vm_throw
cti_vm_throw PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_vm_throw
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_vm_throw ENDP

    EXPORT cti_to_object
    IMPORT JITStubThunked_to_object
cti_to_object PROC
    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bl JITStubThunked_to_object
    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET]
    bx lr
cti_to_object ENDP

    END
