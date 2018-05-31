SAMPLE_SIZE = 100
TOTAL_SIZE = 1000
training_x = []
training_y = []
testing_x = []
testing_y = []
filepath = 'sample.csv'

with open(filepath) as fp:
  # iterate past first line
  fp.readline()
  
  # collect training data
  line_raw = fp.readline()
  while line_raw and line_raw.isdigit():
    # collect output (n x 1)
    line = map(int,line_raw[:len(line)-1].split(',',256))
    training_y.append(line)
    # data_raw = []
    data_scaled_up = []
    data_scaled_down = []
    for i in range(TOTAL_SIZE):
      line_raw = fp.readline()
      line = map(int,line_raw[:len(line)-1].split(',',256))
      data_raw.append(line)
      # data synthesis
      data_scaled_up.append(map(lambda x: x*1.2, line))
      data_scaled_down.append(map(lambda x: x*0.8, line))
    for i in range (TOTAL_SIZE-SAMPLE_SIZE):
      training_x.append(data_raw[i:SAMPLE_SIZE+i])
      training_x.append(data_scaled_up[i:SAMPLE_SIZE+i])
      training_x.append(data_scaled_down[i:SAMPLE_SIZE+i])
    line_raw = fp.readline()

  # collect testing data
  line_raw = fp.readline()
  while line:
    # collect output (n x 1)
    line = map(int,line_raw[:len(line)-1].split(',',256))
    testing_y.append(line)
    # collect input (3 x SAMPLE_SIZE)
    data_raw = []
    for i in range(SAMPLE_SIZE):
      line_raw = fp.readline()
      line = map(int,line_raw[:len(line)-1].split(',',256))
      data_raw.append(line)
    testing_x.append(data_raw)
    line_raw = fp.readline()
