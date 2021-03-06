#include "neuralnetwork.h"

double Neuron::eta = 0.15;
double Neuron::alpha = 0.5;

Neuron::Neuron (unsigned num_outputs, unsigned my_index) {
    for (unsigned c=0;c<num_outputs;++c) {
        output_weights.push_back(Connection());
        //output_weights.back().delta_weight = 0.0;
        output_weights.back().weight = random_weight();
    }
    index = my_index;
}

void Neuron::set_weights(unsigned num_neurons) {
    double x = 1.0 / sqrt(num_neurons);
    for (unsigned c=0;c<output_weights.size();++c) {
        double r = (rand() / (double) RAND_MAX)*2*x - x;
        output_weights[c].weight = r;
    }
    
} 

void Neuron::update_input_weights(Layer &prev_layer) {
    for (Layer::iterator it = prev_layer.begin();it!=prev_layer.end();++it) {
        Neuron &neuron = *it;
        double old_delta_weight = neuron.output_weights.at(index).delta_weight;

        double new_delta_weight = eta * neuron.get_output_value() * gradient + alpha * old_delta_weight;
        neuron.output_weights[index].delta_weight = new_delta_weight;
        neuron.output_weights[index].weight += new_delta_weight;
    } 
}

double Neuron::transfer_function(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double Neuron::transfer_function_derivative(double x) {
    double f = Neuron::transfer_function(x);
    return f * (1-f);
}

void Neuron::feed_forward(const Layer &prev_layer) {
    //cout << "Neuron " << index << endl;
    double sum = 0.0;
    for (unsigned n=0; n<prev_layer.size(); ++n) {
        //cout << prev_layer[n].get_output_value() << " * " << prev_layer[n].output_weights[index].weight << " = " << prev_layer[n].get_output_value() * prev_layer[n].output_weights[index].weight << endl;
        sum += prev_layer[n].get_output_value() * prev_layer[n].output_weights[index].weight;
    }
    output_val = Neuron::transfer_function(sum);
}

double Neuron::sum_DOW(const Layer &next_layer) const {
    double sum = 0.0;
    for (unsigned n=0; n<next_layer.size() - 1; ++n ) { 
        //cout << output_weights[n].weight << " " << next_layer[n].gradient << endl;
        sum += output_weights[n].weight * next_layer[n].gradient;
    }
    return sum;
}

void Neuron::calc_hidden_gradients(const Layer &next_layer) {
    double dow = sum_DOW(next_layer);
    gradient = dow * Neuron::transfer_function_derivative(output_val);
}

void Neuron::calc_output_gradients(double target_val) {
    double delta = target_val - output_val;
    gradient = delta * Neuron::transfer_function_derivative(output_val);
}

void Neuron::set_output_value(double val) {
    output_val = val;
}


void Net::get_results(vector<double> &result_vals) const {
    /*for (unsigned i=0;i<layers.size();i++) {
        cout << "Layer " << i << ": {";
        for (unsigned n=0;n<layers[i].size();n++) {
            cout << layers[i][n].get_output_value() << " ";
        }
        cout << "}" << endl;
    }*/
    cout << "{ ";
    for (unsigned i=0;i<layers[1].size();i++) {
        cout << layers[1][i].get_output_value() << " ";
    }
    cout << "}" << endl;


    result_vals.clear();
    for (unsigned n=0;n<layers.back().size() -1;++n) {
        result_vals.push_back(layers.back()[n].get_output_value());
    }
}

void Net::feed_forward(const vector<double> &input_vals) {


    assert(input_vals.size() == layers[0].size()-1);
    for (unsigned i=0; i<input_vals.size();++i) {
        layers[0][i].set_output_value(input_vals[i]);
    }

    /*cout << "{ ";
    for (unsigned i=0;i<layers[0].size();++i) {
        cout << layers[0][i].get_output_value() << " ";
    }
    cout << "}" << endl;*/


    for (unsigned layer_num = 1; layer_num < layers.size(); ++layer_num) {
        Layer &prev_layer = layers[layer_num - 1];
        cout << "Layer " << layer_num << ":" << endl;
        for (unsigned n = 0; n < layers[layer_num].size() - 1; ++n) {
            layers[layer_num][n].feed_forward(prev_layer);
        }
    }
}

void Net::backprop(const vector<double> &target_vals) {
    Layer &output_layer = layers.back(); 
    cout << "outputs: " << output_layer.size() << ", targets: " << target_vals.size() << endl;
    error = 0.0;
    for (unsigned n=0; n<output_layer.size()-1;++n) {
        double delta = target_vals[n] - output_layer[n].get_output_value();
        error += delta*delta;
    }
    error /= output_layer.size() - 1;
    error = sqrt(error);

    for (unsigned n =0; n< output_layer.size() - 1; ++n) {
        output_layer[n].calc_output_gradients(target_vals[n]);
    }

    for (unsigned layer_num = layers.size()-2; layer_num > 0; layer_num--) {
        Layer &hidden_layer = layers[layer_num];
        Layer &next_layer = layers[layer_num+1];

        for (unsigned n=0;n<hidden_layer.size();++n) {
            //cout << "LAYER " << layer_num << ", NEURON " << n << ":"<<endl;
            hidden_layer[n].calc_hidden_gradients(next_layer);
        }
    }

    for (unsigned layer_num = layers.size() - 1; layer_num >= 1; layer_num--) {
        cout << "updating layer "<<layer_num << endl;
        Layer &layer = layers[layer_num];
        Layer &prev_layer = layers[layer_num-1];
        for (unsigned n=0;n<layer.size()-1;++n) {
            layer[n].update_input_weights(prev_layer);
        }
    }
}

vector<unsigned> Net::get_topology() {
    return top;
}

void Net::save_to_file(const char * filename) {
    cout << top.size() << endl;
    ofstream ofs(filename);
    ofs << top.size() << endl;     
    for (unsigned i=0;i<top.size();i++) {
        ofs << top[i] << " ";
    }
    ofs << endl;
    for (unsigned layer_num = 0; layer_num < layers.size()-1; layer_num++) {
        for (unsigned neuron_num = 0; neuron_num < layers[layer_num].size(); neuron_num++) {
            Neuron curr_neuron = layers[layer_num][neuron_num];
            for (unsigned weight_num = 0; weight_num < curr_neuron.output_weights.size()-1;weight_num++) {
                Connection curr_weight = curr_neuron.output_weights[weight_num];
                ofs << curr_weight.weight << " " << curr_weight.delta_weight << ",";
            }
            Connection last_weight = curr_neuron.output_weights.back();
            ofs << last_weight.weight << " " << last_weight.delta_weight << endl;
        }          
    } 
    ofs.close();
}

Net::Net(const vector<unsigned> &topology) {
    vector<unsigned> x(topology);
    top = x;
    unsigned num_layers = topology.size();
    for (unsigned i = 0; i<num_layers; ++i) {
        layers.push_back(Layer());
        unsigned num_outputs = i == topology.size() - 1? 0:topology[i+1];

        for (unsigned neuron_num = 0; neuron_num <= topology[i]; ++neuron_num) {
            layers.back().push_back(Neuron(num_outputs,neuron_num));
            //layers.back().back().set_weights(topology[i]);

        }
        layers.back().back().set_output_value(1.0);
    }
}

Net::Net(const char * filename) {
    ifstream ifs(filename,std::ifstream::in);
    string line;
    getline(ifs,line);
    istringstream ss(line);
    unsigned top_size;
    ss >> top_size;
    getline(ifs,line);
    stringstream top_stream(line);
    for (unsigned i=0;i<top_size;i++) {
        string item;
        while (getline(top_stream,item,' ')) {
            stringstream item_stream(item);
            unsigned layer_size;        
            item_stream >> layer_size;
            top.push_back(layer_size); 
        }
    }
    for (unsigned layer_num=0;layer_num < top.size();layer_num++) {
        layers.push_back(Layer());
        unsigned num_outputs = layer_num == top.size()-1 ? 0 : top[layer_num+1];
        for (unsigned neuron_num = 0;neuron_num <= top[layer_num]; ++neuron_num) {
            layers.back().push_back(Neuron(num_outputs,neuron_num));
            string neuron_line;
            getline(ifs,neuron_line);    
            stringstream neuron_stream(neuron_line);
            string output_weight_string;
            vector<Connection> o_weights;
            while (getline(neuron_stream,output_weight_string,',')) {
                stringstream output_weight_stream(output_weight_string);
                string weight_string;
                string delta_weight_string;
                double c_weight;
                double c_delta_weight;

                getline(output_weight_stream,weight_string,' ');
                stringstream weight_stream(weight_string);
                weight_stream >> c_weight;

                getline(output_weight_stream,delta_weight_string,' ');
                stringstream delta_weight_stream(delta_weight_string);
                delta_weight_stream >> c_delta_weight;

                Connection output_weight = { .weight = c_weight, .delta_weight = c_delta_weight };
                o_weights.push_back(output_weight);
            }
            layers.back().back().output_weights = o_weights;  
        }
        layers.back().back().set_output_value(1.0);
    }
    ifs.close();
}
