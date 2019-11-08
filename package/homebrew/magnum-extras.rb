class MagnumExtras < Formula
  desc "Extras for the Magnum C++11/C++14 graphics engine"
  homepage "https://magnum.graphics"
  url "https://github.com/mosra/magnum-extras/archive/v2019.10.tar.gz"
  # wget https://github.com/mosra/magnum-extras/archive/v2019.10.tar.gz -O - | sha256sum
  sha256 "74d6d2800ef3755899af0f012e6de006fdffba77080215748edcf1911ac1c770"
  head "git://github.com/mosra/magnum-extras.git"

  depends_on "cmake"
  depends_on "magnum"

  def install
    system "mkdir build"
    cd "build" do
      system "cmake", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_INSTALL_PREFIX=#{prefix}", "-DWITH_PLAYER=ON", "-DWITH_UI=ON", "-DWITH_UI_GALLERY=ON", ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
