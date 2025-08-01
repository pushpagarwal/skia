               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %_entrypoint_v "_entrypoint" %sk_FragColor
               OpExecutionMode %_entrypoint_v OriginUpperLeft

               ; Debug Information
               OpName %sk_FragColor "sk_FragColor"  ; id %3
               OpName %_UniformBuffer "_UniformBuffer"  ; id %8
               OpMemberName %_UniformBuffer 0 "colorGreen"
               OpName %_entrypoint_v "_entrypoint_v"    ; id %10
               OpName %main "main"                      ; id %2

               ; Annotations
               OpDecorate %main RelaxedPrecision
               OpDecorate %sk_FragColor RelaxedPrecision
               OpDecorate %sk_FragColor Location 0
               OpDecorate %sk_FragColor Index 0
               OpMemberDecorate %_UniformBuffer 0 Offset 0
               OpMemberDecorate %_UniformBuffer 0 RelaxedPrecision
               OpDecorate %_UniformBuffer Block
               OpDecorate %7 Binding 0
               OpDecorate %7 DescriptorSet 0
               OpDecorate %27 RelaxedPrecision
               OpDecorate %28 RelaxedPrecision
               OpDecorate %30 RelaxedPrecision
               OpDecorate %32 RelaxedPrecision
               OpDecorate %33 RelaxedPrecision
               OpDecorate %35 RelaxedPrecision
               OpDecorate %36 RelaxedPrecision
               OpDecorate %37 RelaxedPrecision
               OpDecorate %38 RelaxedPrecision
               OpDecorate %39 RelaxedPrecision

               ; Types, variables and constants
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
%sk_FragColor = OpVariable %_ptr_Output_v4float Output  ; RelaxedPrecision, Location 0, Index 0
%_UniformBuffer = OpTypeStruct %v4float                 ; Block
%_ptr_Uniform__UniformBuffer = OpTypePointer Uniform %_UniformBuffer
          %7 = OpVariable %_ptr_Uniform__UniformBuffer Uniform  ; Binding 0, DescriptorSet 0
       %void = OpTypeVoid
         %12 = OpTypeFunction %void
    %float_0 = OpConstant %float 0
    %v2float = OpTypeVector %float 2
         %16 = OpConstantComposite %v2float %float_0 %float_0
%_ptr_Function_v2float = OpTypePointer Function %v2float
         %20 = OpTypeFunction %v4float %_ptr_Function_v2float
%_ptr_Uniform_v4float = OpTypePointer Uniform %v4float
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0


               ; Function _entrypoint_v
%_entrypoint_v = OpFunction %void None %12

         %13 = OpLabel
         %17 =   OpVariable %_ptr_Function_v2float Function
                 OpStore %17 %16
         %19 =   OpFunctionCall %v4float %main %17
                 OpStore %sk_FragColor %19
                 OpReturn
               OpFunctionEnd


               ; Function main
       %main = OpFunction %v4float None %20         ; RelaxedPrecision
         %21 = OpFunctionParameter %_ptr_Function_v2float

         %22 = OpLabel
         %23 =   OpAccessChain %_ptr_Uniform_v4float %7 %int_0
         %27 =   OpLoad %v4float %23                ; RelaxedPrecision
         %28 =   OpCompositeExtract %float %27 0    ; RelaxedPrecision
         %29 =   OpConvertFToS %int %28
         %30 =   OpConvertSToF %float %29           ; RelaxedPrecision
         %31 =   OpAccessChain %_ptr_Uniform_v4float %7 %int_0
         %32 =   OpLoad %v4float %31                ; RelaxedPrecision
         %33 =   OpCompositeExtract %float %32 1    ; RelaxedPrecision
         %34 =   OpAccessChain %_ptr_Uniform_v4float %7 %int_0
         %35 =   OpLoad %v4float %34                ; RelaxedPrecision
         %36 =   OpVectorShuffle %v2float %35 %35 2 3   ; RelaxedPrecision
         %37 =   OpCompositeExtract %float %36 0        ; RelaxedPrecision
         %38 =   OpCompositeExtract %float %36 1        ; RelaxedPrecision
         %39 =   OpCompositeConstruct %v4float %30 %33 %37 %38  ; RelaxedPrecision
                 OpReturnValue %39
               OpFunctionEnd
