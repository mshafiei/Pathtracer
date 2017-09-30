#include <cassert>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <memory>

#include "bvh.h"
#include "bbox.h"

inline void flag_primitives(const int* prims, int begin, int end, uint8_t* flags, int split) {
    for (int i = begin; i < split; i++) {
        flags[prims[i]] = 0;
    }

    for (int i = split; i < end; i++) {
        flags[prims[i]] = 1;
    }
}

inline void sorted_partition(int* prims, int* tmp, int begin, int end, int split, const uint8_t* flags) {
    int left = begin, right = split;
    for (int i = begin; i < end; i++) {
        if (flags[prims[i]]) {
            tmp[right++] = prims[i];
        } else {
            tmp[left++]  = prims[i];
        }
    }

    std::copy(tmp + begin, tmp + end, prims + begin);
}

static int find_split(const int* prims, float* tmp_cost, int begin, int end, const BBox* bboxes, BBox& right_bb, float& cost) {
    BBox cur_bb = BBox::empty();

    // Sweep from the left and compute costs
    for (int i = begin; i < end - 1; i++) {
        cur_bb = extend(cur_bb, bboxes[prims[i]]);
        tmp_cost[i] = (i - begin + 1) * half_area(cur_bb);
    }

    float min_cost = FLT_MAX;
    int min_split = -1;
    BBox min_bb;

    cur_bb = BBox::empty();

    // Sweep from the right and find the minimum cost
    for (int i = end - 1; i > begin; i--) {
        cur_bb = extend(cur_bb, bboxes[prims[i]]);

        const float c = tmp_cost[i - 1] + (end - i) * half_area(cur_bb);
        if (c < min_cost) {
            min_bb = cur_bb;
            min_cost = c;
            min_split = i;
        }
    }

    right_bb = min_bb;
    cost     = min_cost;
    return min_split;
}

struct BvhBuilder {
    BvhBuilder(const BBox* bboxes,
               uint8_t* tmp_flags, int** prims,
               Bvh::Node* nodes,
               int& node_count)
        : bboxes(bboxes)
        , tmp_flags(tmp_flags)
        , prims(prims)
        , nodes(nodes)
        , node_count(node_count)
    {}

    void build(int node_id) {
        const float traversal_cost = 1.0f;

        Bvh::Node& node = nodes[node_id];
        const int begin = node.first_prim;
        const int end   = node.first_prim + node.num_prims;

        if (end - begin <= 1)
            return;

        int*   tmp_prims = tmp_storage.get() - begin;
        float* tmp_costs = (float*)tmp_prims; // Save some storage and re-use tmp buffer

        // On all three axes, try to split this node
        BBox  right_bb, min_right;
        float min_cost  = FLT_MAX, cost;
        int   min_split = -1;
        int   min_axis;

        for (int i = 0; i < 3; i++) {
            const int split = find_split(prims[i], tmp_costs, begin, end, bboxes, right_bb, cost);
            if (cost < min_cost) {
                min_right = right_bb;
                min_cost  = cost;
                min_split = split;
                min_axis  = i;
            }
        }

        assert(min_split > begin && min_split < end);

        // Compare the minimum split cost with the SAH of this node
        if (min_cost < ((end - begin) - traversal_cost) * half_area(node.min, node.max)) {
            const int axis1 = (min_axis + 1) % 3;
            const int axis2 = (min_axis + 2) % 3;

            flag_primitives(prims[min_axis], begin, end, tmp_flags, min_split);
            sorted_partition(prims[axis1], tmp_prims, begin, end, min_split, tmp_flags);
            sorted_partition(prims[axis2], tmp_prims, begin, end, min_split, tmp_flags);

            // Recompute the bounding box of the left child
            BBox min_left = BBox::empty();
            for (int i = begin; i < min_split; i++) {
                min_left = extend(min_left, bboxes[prims[min_axis][i]]);
            }

            int num_nodes;

            //#pragma omp atomic capture
            {num_nodes = node_count; node_count += 2;}

            // Mark the node as an inner node
            node.child = num_nodes;
            node.axis = -min_axis;
            
            // Setup the child nodes
            Bvh::Node& left = nodes[num_nodes];
            left.first_prim = begin;
            left.num_prims  = min_split - begin;
            left.min = min_left.min;
            left.max = min_left.max;

            Bvh::Node& right = nodes[num_nodes + 1];
            right.first_prim = min_split;
            right.num_prims  = end - min_split;
            right.min = min_right.min;
            right.max = min_right.max;

            const int smallest_node = right.num_prims <  left.num_prims ? num_nodes + 1 : num_nodes;
            const int biggest_node  = right.num_prims >= left.num_prims ? num_nodes + 1 : num_nodes;

            bool spawn_task = nodes[smallest_node].num_prims > parallel_threshold();
            if (spawn_task) {
                BvhBuilder* builder = new BvhBuilder(bboxes, tmp_flags, prims, nodes, node_count);
                builder->allocate_tmp_storage(nodes[smallest_node].num_prims);
                //#pragma omp task firstprivate(builder, smallest_node)
                {
                    builder->build_and_delete(smallest_node);
                }
            }

            build(biggest_node);
            if (!spawn_task) build(smallest_node);
        }
    }

    void build_and_delete(int node_id) {
        build(node_id);
        delete this;
    }

    void allocate_tmp_storage(int num_tris) {
        tmp_storage.reset(new int[num_tris]);
    }

    static constexpr int parallel_threshold() { return 1000; }

    const BBox* bboxes;

    std::unique_ptr<int[]> tmp_storage;
    uint8_t* tmp_flags;
    int** prims;

    Bvh::Node* nodes;
    int& node_count;
};

void Bvh::build(const BBox* bboxes, const float3* centers, int num_tris) {
    Node& root = nodes[0];
    root.min = float3(FLT_MAX);
    root.max = float3(-FLT_MAX);
    root.first_prim = 0;
    root.num_prims  = num_tris;
    num_nodes = 1;

    // Compute global bounding box
    #pragma omp parallel
    {
        BBox local = BBox::empty();
        #pragma omp for nowait
        for (int i = 0; i < num_tris; i++) {
            local.min = min(bboxes[i].min, local.min);
            local.max = max(bboxes[i].max, local.max);
        }
        #pragma omp critical
        {
            root.min = min(local.min, root.min);
            root.max = max(local.max, root.max);
        }
    }

    prim_ids.reset(new int[num_tris]);
    std::unique_ptr<int[]> all_prims(new int[2 * num_tris]);
    int* prims[3] = { prim_ids.get(), all_prims.get(), all_prims.get() + num_tris };
    #pragma omp parallel for
    for (int i = 0; i < num_tris; i++) {
        prims[0][i] = i;
        prims[1][i] = i;
        prims[2][i] = i;
    }

    std::unique_ptr<uint8_t[]> tmp_flags(new uint8_t[num_tris]);

    BvhBuilder* builder = new BvhBuilder(bboxes, tmp_flags.get(), prims, nodes.get(), num_nodes);
    builder->allocate_tmp_storage(num_tris);

    #pragma omp parallel
    {
        // Sort according to projection of barycenter on each axis
        #pragma omp sections
        {
            #pragma omp section
            std::sort(prims[0], prims[0] + num_tris, [&] (int p0, int p1) { return centers[p0].x < centers[p1].x; });

            #pragma omp section
            std::sort(prims[1], prims[1] + num_tris, [&] (int p0, int p1) { return centers[p0].y < centers[p1].y; });

            #pragma omp section
            std::sort(prims[2], prims[2] + num_tris, [&] (int p0, int p1) { return centers[p0].z < centers[p1].z; });
        }

        #pragma omp barrier

        #pragma omp single
        builder->build_and_delete(0);
    }

    // Resize the array of nodes
    Node* tmp_nodes = new Node[num_nodes];
    std::copy(nodes.get(), nodes.get() + num_nodes, tmp_nodes);
    nodes.reset(tmp_nodes);
}

void Bvh::build(const float3* verts, const int* indices, int num_tris) {
    // Compute per-triangle and global bounding box
    std::unique_ptr<BBox[]>   bboxes(new BBox[num_tris]);
    std::unique_ptr<float3[]> centers(new float3[num_tris]);

    nodes.reset(new Node[num_tris * 2 + 1]);

    #pragma omp parallel for
    for (int i = 0; i < num_tris; i++) {
        const float3 v0 = verts[indices[i * 4 + 0]];
        const float3 v1 = verts[indices[i * 4 + 1]];
        const float3 v2 = verts[indices[i * 4 + 2]];

        centers[i] = (1.0f / 3.0f) * (v0 + v1 + v2);
        bboxes[i].min = min(v0, min(v1, v2));
        bboxes[i].max = max(v0, max(v1, v2));
    }

    build(bboxes.get(), centers.get(), num_tris);

    tris.reset(new PrecomputedTri[num_tris]);

    #pragma omp parallel for
    for (int i = 0; i < num_tris; i++) {
        int tri_id = prim_ids[i];
        int i0 = indices[tri_id * 4 + 0];
        int i1 = indices[tri_id * 4 + 1];
        int i2 = indices[tri_id * 4 + 2];
        tris[i] = PrecomputedTri(verts[i0], verts[i1], verts[i2]);
    }
}

void Bvh::traverse(const Ray& ray, Hit& hit, bool any) const {
    constexpr int stack_size = 64;
    int stack[stack_size];
    int top = 1;
    int stack_ptr = 0;

    hit.tri = -1;
    hit.t = ray.tmax;
    hit.u = 0;
    hit.v = 0;

    int ox = ray.dir.x > 0 ? 0 : 4;
    int oy = ray.dir.y > 0 ? 1 : 5;
    int oz = ray.dir.z > 0 ? 2 : 6;
    auto idir = float3(1.0f) / ray.dir;
    auto oidir = ray.org * idir;

    stack[0] = -1;
    while (top >= 0) {
        auto& left  = nodes[top + 0];
        auto& right = nodes[top + 1];

        // Intersect the two children of this node
        auto p0 = reinterpret_cast<const float*>(&left);
        auto p1 = reinterpret_cast<const float*>(&right);
        auto t00x = p0[    ox] * idir.x - oidir.x;
        auto t10x = p1[    ox] * idir.x - oidir.x;
        auto t00y = p0[    oy] * idir.y - oidir.y;
        auto t10y = p1[    oy] * idir.y - oidir.y;
        auto t00z = p0[    oz] * idir.z - oidir.z;
        auto t10z = p1[    oz] * idir.z - oidir.z;
        auto t01x = p0[4 - ox] * idir.x - oidir.x;
        auto t11x = p1[4 - ox] * idir.x - oidir.x;
        auto t01y = p0[6 - oy] * idir.y - oidir.y;
        auto t11y = p1[6 - oy] * idir.y - oidir.y;
        auto t01z = p0[8 - oz] * idir.z - oidir.z;
        auto t11z = p1[8 - oz] * idir.z - oidir.z;
        float t0[2], t1[2];
        t0[0] = std::max(std::max(ray.tmin, t00x), std::max(t00y, t00z));
        t0[1] = std::max(std::max(ray.tmin, t10x), std::max(t10y, t10z));
        t1[0] = std::min(std::min(hit.t, t01x), std::min(t01y, t01z));
        t1[1] = std::min(std::min(hit.t, t11x), std::min(t11y, t11z));

        const int old_ptr = stack_ptr;

        auto intersect_leaf = [&] (const Node& leaf) {
            for (int j = leaf.first_prim; j < leaf.first_prim + leaf.num_prims; j++) {
                if (intersect_ray_tri(ray, tris[j], hit.t, hit.u, hit.v)) {
                    hit.tri = j;
                    if (any) return;
                }
            }
        };

        if (t0[0] <= t1[0]) {
            if (left.num_prims > 0) intersect_leaf(left);
            else stack[++stack_ptr] = left.child;
        }

        if (t0[1] <= t1[1]) {
            if (right.num_prims > 0) intersect_leaf(right);
            else stack[++stack_ptr] = right.child;
        }

        // Reorder the children on the stack
        if (old_ptr + 2 <= stack_ptr && t0[0] < t0[1])
            std::swap(stack[stack_ptr], stack[stack_ptr - 1]);

        top = stack[stack_ptr--];
    }

    if (hit.tri >= 0) hit.tri = prim_ids[hit.tri];
}
