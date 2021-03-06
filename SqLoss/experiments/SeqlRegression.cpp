/*
 * Author: Severin Gsponer (severin.gsponer@insight-centre.org)
 * Programm to start the regression pipeline
 * At least provide training file, testfile and a basename for all the output files. 
 */

#include "../seql_learn.h"
#include "../seql_predictor.h"
#include "../FileReader.cpp"

#define OPT "[-t theshold] [-o objective_function] [-O csvFile] [-m minsup] [-l minpat] [-L maxpat] [-g maxgap] [-G maxcongap] [-r traversal_strategy ] [-T #round] [-n token_type] [-c convergence_threshold] [-C regularizer_value] [-a l1_vs_l2_regularizer_weight] [-v verbosity] training_file test_file basename"

int main (int argc, char **argv)
{
    unsigned int objective = 3;
    // Word or character token type. By default char token.
    bool token_type = 1;

    // Pattern properties
    unsigned int maxpat = 0xffffffff;
    unsigned int minpat = 1;
    unsigned int maxitr = 5000;
    unsigned int minsup = 1;
    // Max # of total wildcards allowed in a feature.
    unsigned int maxgap = 0;
    // Max # of consec wildcards allowed in a feature.
    unsigned int maxcongap = 0;

    // BFS vs DFS traversal. By default BFS.
    bool traversal_strategy = 0;

    // The C regularizer parameter in regularized loss formulation. It constraints the weights of features.
    // C = 0 no constraints (standard SLR), the larger the C, the more the weights are shrinked towards each other (using L2) or towards 0 (using L1)
    double C = 1;
    // The alpha parameter decides weight on L1 vs L2 regularizer: alpha * L1 + (1 - alpha) * L2. By default we use an L2 regularizer.
    double alpha = 0.2;

    double convergence_threshold = 0.005;
    int verbosity = 0;

    std::string csvFile;
    bool csvLog = false;

    double mean = 0;

    int opt;
    while ((opt = getopt(argc, argv, "t:o:T:L:l:m:g:G:n:r:c:C:a:v:O:")) != -1) {
        switch(opt) {
        case 'o':
            objective = atoi (optarg);
            break;
        case 'T':
            maxitr = atoi (optarg);
            break;
        case 'L':
            maxpat = atoi (optarg);
            break;
        case 'l':
            minpat = atoi (optarg);
            break;
        case 'm':
            minsup = atoi (optarg);
            break;
        case 'g':
            maxgap = atoi (optarg);
            break;
        case 'G':
            maxcongap = atoi (optarg);
            break;
        case 'n':
            token_type = atoi (optarg);
            break;
        case 'r':
            traversal_strategy = atoi (optarg);
            break;
        case 'c':
            convergence_threshold = atof (optarg);
            break;
        case 'C':
            C = atof (optarg);
            break;
        case 'a':
            alpha = atof (optarg);
            break;
        case 'v':
            verbosity = atoi (optarg);
            break;
        case 't':
            mean = atof(optarg);
            break;
        case 'O':
            csvFile = optarg;
            csvLog = true;
            break;
        default:
            std::cout << "Usage: " << argv[0] << OPT << std::endl;
            return -1;
        }
    }

    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << OPT << std::endl;
        return -1;
    }
    std::string inputFile{argv[argc-4]};
    std::string validationSet{argv[argc-1]};
    std::string basename = {argv[argc-2]};
    // const char* modelCreationFile{basename + ".model"};
    std::string modelCreationFile{basename + ".modelCreation"};
    std::string modelBinFile{basename + ".bin"};
    std::string modelFile{basename + ".model"};
    std::string predictionFile{basename + ".conc.pred"};
    std::string statsfile{basename + ".stats.json"};
    std::string testFile{argv[argc-3]};

    std::vector<std::string> x;
    std::vector<double> y;
    std::tie(x,y, mean) = SEQL::read_input(inputFile);
    // substract mean
    for (auto& ele : y){
        ele -= mean;
    }

    std::vector<std::string> xval;
    std::vector<double> yval;
    std::tie(xval,yval, std::ignore) = SEQL::read_input(validationSet);

    SeqLearner seql_learner {x,y,
                            objective,
                            maxpat,
                            minpat,
                            minsup,
                            maxgap,
                            maxcongap,
                            token_type,
                            traversal_strategy,
                            convergence_threshold,
                            C,
                            alpha,
                            verbosity,
                            csvLog};
    auto start = std::chrono::steady_clock::now();
    LinearModel::LinearModel model =  seql_learner.learn (maxitr, modelCreationFile, mean, xval, yval, csvFile);
    auto end = std::chrono::steady_clock::now();
    cout << "\nTotal time taken to run learner: "
         << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
         << "ms"<< endl;
    model.print_fulllist(modelCreationFile);
    model.build_tree(1);
    model.set_bias(mean);
    model.save_as_binary(modelBinFile);
    model.print_model(modelFile);

    SEQLPredictor predictor{1, token_type, &model};
    // // classifier.open(modelBinFile, 0);
    predictor.evalFile(testFile, predictionFile );
    predictor.print_reg_stats(statsfile);
    return 0;
}
