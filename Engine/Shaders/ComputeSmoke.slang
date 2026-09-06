[[vk::binding(0, 0)]] RWStructuredBuffer<uint> output_values;

[shader("compute")]
    [numthreads(1, 1, 1)] void
    main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
	    output_values[dispatch_thread_id.x] = dispatch_thread_id.x;
    }
