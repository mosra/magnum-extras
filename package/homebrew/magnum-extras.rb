class MagnumExtras < Formula
  desc "Extras for the Magnum C++11/C++14 graphics engine"
  homepage "https://magnum.graphics"
  url "https://github.com/mosra/magnum-extras/archive/v2019.01.tar.gz"
  # wget https://github.com/mosra/magnum-extras/archive/v2019.01.tar.gz -O - | sha256sum
  sha256 "eac4e6874323e588c40a4ca20d80a4fcf16ccad2f8a7651f5f0b60d2a9b9c907"
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
