#ifndef WAVE_GENERATOR_H_
#define WAVE_GENERATOR_H_

class WaveGenerator {
 public:
  WaveGenerator();
  virtual ~WaveGenerator();
  std::string GetUrl();

 private:
  std::string url_;
};

#endif

