# kate: indent-width 2;

class MagnumExtras < Formula
  desc "Extras for Magnum graphics engine"
  homepage "https://github.com/mosra/magnum"
  head "git://github.com/mosra/magnum-extras.git"

  depends_on "cmake"
  depends_on "magnum"

  def install
    system "mkdir build"
    cd "build" do
      system "cmake", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_INSTALL_PREFIX=#{prefix}", "-DWITH_UI=ON", ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
