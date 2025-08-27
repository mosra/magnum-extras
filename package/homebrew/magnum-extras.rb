class MagnumExtras < Formula
  desc "Extras for the Magnum C++11 graphics engine"
  homepage "https://magnum.graphics"
  # git describe origin/master, except the `v` prefix
  version "2020.06-865-g54669108e2"
  # Clone instead of getting an archive to have tags for version.h generation
  url "https://github.com/mosra/magnum-extras.git", revision: "54669108e2"
  head "https://github.com/mosra/magnum-extras.git"

  depends_on "cmake" => :build
  depends_on "magnum"
  depends_on "magnum-plugins"

  def install
    system "mkdir build"
    cd "build" do
      system "cmake",
        *std_cmake_args,
        # Without this, ARM builds will try to look for dependencies in
        # /usr/local/lib and /usr/lib (which are the default locations) instead
        # of /opt/homebrew/lib which is dedicated for ARM binaries. Please
        # complain to Homebrew about this insane non-obvious filesystem layout.
        "-DCMAKE_INSTALL_NAME_DIR:STRING=#{lib}",
        "-DMAGNUM_WITH_PLAYER=ON",
        "-DMAGNUM_WITH_UI=ON",
        "-DMAGNUM_WITH_UI_GALLERY=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
