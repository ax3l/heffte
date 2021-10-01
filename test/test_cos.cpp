/** @class */
/*
    -- heFFTe --
       Univ. of Tennessee, Knoxville
       @date
*/

#include "test_fft3d.h"

template<typename backend_tag, typename scalar_type>
void test_cosine_transform(MPI_Comm comm){
    using tvector = typename heffte::fft3d<backend_tag>::template buffer_container<scalar_type>; // std::vector or cuda::vector

    int const me = mpi::comm_rank(comm);
    int const num_ranks = mpi::comm_size(comm);
    assert(num_ranks == 4);
    current_test<scalar_type, using_mpi, backend_tag> name(std::string("-np ") + std::to_string(num_ranks) + "  test cosine", comm);

    box3d<> const world = {{0, 0, 0}, {1, 2, 3}};
    std::vector<scalar_type> world_input(world.count());
    std::iota(world_input.begin(), world_input.end(), 1.0);
    std::vector<scalar_type> world_result = {2.4e+03, -6.7882250993908571e+01, -2.2170250336881628e+02, 0.0, 0.0, 0.0, -9.0844474461089760e+02, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -6.4561180200187039e+01, 0.0, 0.0, 0.0, 0.0, 0.0};

    std::vector<box3d<>> boxes = heffte::split_world(world, std::array<int, 3>{1, 2, 2});
    assert(boxes.size() == static_cast<size_t>(num_ranks));
    auto local_input = input_maker<backend_tag, scalar_type>::select(world, boxes[me], world_input);
    auto reference = get_subbox(world, boxes[me], world_result);
    auto reference_inv = get_subbox(world, boxes[me], world_input);

    for(auto const options : make_all_options<backend_tag>()){
        if (not options.use_pencils) continue;
        heffte::fft3d<backend_tag> trans_cos(boxes[me], boxes[me], comm, options);
        tvector forward(trans_cos.size_outbox());

        trans_cos.forward(local_input.data(), forward.data());
        tassert(approx(forward, reference));

        tvector inverse(trans_cos.size_inbox());
        trans_cos.backward(forward.data(), inverse.data(), heffte::scale::full);
        tassert(approx(inverse, reference_inv, (std::is_same<scalar_type, float>::value) ? 0.001 : 1.0));
    }
}


void perform_tests(MPI_Comm const comm){
    all_tests<> name("cosine transforms");

    test_cosine_transform<backend::stock_cos, float>(comm);
    test_cosine_transform<backend::stock_cos, double>(comm);
    #ifdef Heffte_ENABLE_FFTW
    test_cosine_transform<backend::fftw_cos, float>(comm);
    test_cosine_transform<backend::fftw_cos, double>(comm);
    #endif
    #ifdef Heffte_ENABLE_MKL
    test_cosine_transform<backend::mkl_cos, float>(comm);
    test_cosine_transform<backend::mkl_cos, double>(comm);
    #endif
}

int main(int argc, char *argv[]){

    MPI_Init(&argc, &argv);

    perform_tests(MPI_COMM_WORLD);

    MPI_Finalize();

    return 0;
}
