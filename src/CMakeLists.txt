add_executable(arty
    main.cpp
    bvh.cpp
    bvh.h
    load_obj.cpp
    load_obj.h
    image.h
    image.cpp
    lights.h
    materials.h
    cameras.h
    intersect.h
    scene.h
    scene.cpp
    render.h
    samplers.h
    float4.h
    float3.h
    float2.h
    file_path.h
    common.h
    random.h
    options.h
    debug.h
    debug.cpp
    hash_grid.h
    algorithms/render_debug.cpp
    algorithms/render_pt.cpp
    
    )

target_link_libraries(arty ${SDL2_LIBRARY} ${PNG_LIBRARIES} ${YAML_CPP_LIBRARIES})
