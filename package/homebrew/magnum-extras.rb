class MagnumExtras < Formula
  desc "Extras for the Magnum C++11/C++14 graphics engine"
  homepage "https://magnum.graphics"
  url "https://github.com/mosra/magnum-extras/archive/v2020.06.tar.gz"
  # wget https://github.com/mosra/magnum-extras/archive/v2020.06.tar.gz -O - | sha256sum
  sha256 "a8d7babc50ac070984d39f6cc15c3ce2af7b41fe980fe81b0405da6f5ba3c36d"
  head "git://github.com/mosra/magnum-extras.git"

  depends_on "cmake"
  depends_on "magnum"

  def install
    system "mkdir build"
    cd "build" do
      system "cmake",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=#{prefix}",
        "-DWITH_PLAYER=ON",
        "-DWITH_UI=ON",
        "-DWITH_UI_GALLERY=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
