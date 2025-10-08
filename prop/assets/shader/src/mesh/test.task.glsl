#extension GL_EXT_task_shader : require

#define MAX_VERTICES = 64;

layout(local_size_x = 16) in;

struct mesh_dispatch_info{
    uint instruction_offset; //offset in 8 Byte
    uint vertex_offset;
};


layout(set = 0, binding = 0, std430) restrict readonly buffer instr{
    uint64_t buf[];
} instruction;

layout(set = 0, binding = 1) uniform task_rng{
    mesh_dispatch_info buf[33];
} subrange;

//taskPayloadSharedEXT MyPayload {
//    mesh_dispatch_info[16] subrange;
//} payload;

void mesh_dispatch_call(const uint lid){
    const uint gid = gl_WorkGroupID.x;
    mesh_dispatch_info rng_cur = subrange.buf[gid];
    mesh_dispatch_info rng_nxt = subrange.buf[gid + 1];

    gl_cull

//    gl_WorkGroupID.x;
    const auto ptr_to_src = std::assume_aligned<8>(where);
auto ptr_to_head = std::assume_aligned<8>(where);

std::uint32_t currentMeshCount{};
std::uint32_t verticesBreakpoint{};

while(currentMeshCount / MaxMeshDispatchPerTask < MaxTaskDispatchPerTime){
std::uint32_t curMeshPushedVertices{};

const auto ptr_to_chunk_head = ptr_to_head;

auto save_chunk_head_and_incr = [&]{
region_sentinel[currentMeshCount].instruction_offset = (ptr_to_chunk_head - ptr_to_src) / GPU_BUF_UNIT_SIZE;
++currentMeshCount;
};

while(true){
const auto& head = *start_lifetime_as<instruction_head>(ptr_to_head);

switch(head.type){
case draw_instruction_type::noop: [[fallthrough]];
case draw_instruction_type::uniform_update:
save_chunk_head_and_incr();
return;
default: break;
}


auto nextVertices = get_vertex_count(head.type, ptr_to_head + sizeof(instruction_head));
if(verticesBreakpoint){
assert(verticesBreakpoint < nextVertices);
nextVertices -= verticesBreakpoint;
}

if(nextVertices > MaxVerticesPerMesh){
verticesBreakpoint += MaxVerticesPerMesh;
goto PUSH_MESH_EMIT;
}else{
if(curMeshPushedVertices + nextVertices <= MaxVerticesPerMesh){
verticesBreakpoint = 0;
ptr_to_head += head.size;
curMeshPushedVertices += nextVertices;
}else{
verticesBreakpoint += curMeshPushedVertices + nextVertices - MaxVerticesPerMesh;
goto PUSH_MESH_EMIT;
}
}

}

PUSH_MESH_EMIT:
save_chunk_head_and_incr();

//TODO pop until a full command?
if(verticesBreakpoint)region_sentinel[currentMeshCount].vertex_offset = verticesBreakpoint;
}
}

void main() {
    gl
    uint localId = gl_LocalInvocationID.x;

    // 所有32个线程都访问同一个taskPayload实例
    if (localId == 0) {
    taskPayload.transform = calculateTransform();
    }

    barrier(); // 确保写入对其他线程可见

    // 所有线程都可以读取taskPayload.transform
    processData(taskPayload.transform);

    EmitMeshTasksEXT(1, 1, 1);
}