# kate: indent-width 2;

class MagnumExtras < Formula
  desc "Extras for the Magnum C++11/C++14 graphics engine"
  homepage "http://magnum.graphics"
  url "https://github.com/mosra/magnum-extras/archive/v2018.02.tar.gz"
  sha256 "4f78e4cd266c70b849fc92c3ddfe2538506edb4a14d9b495150cc382663d7c2a"
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
