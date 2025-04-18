class MagnumExtras < Formula
  desc "Extras for the Magnum C++11/C++14 graphics engine"
  homepage "https://magnum.graphics"
  url "https://github.com/mosra/magnum-extras/archive/v2020.06.tar.gz"
  # wget https://github.com/mosra/magnum-extras/archive/v2020.06.tar.gz -O - | sha256sum
  sha256 "a8d7babc50ac070984d39f6cc15c3ce2af7b41fe980fe81b0405da6f5ba3c36d"
  head "https://github.com/mosra/magnum-extras.git"

  depends_on "cmake" => :build
  depends_on "magnum"
  depends_on "magnum-plugins"

  def install
    # 2020.06 has the options unprefixed, current master has them prefixed.
    # Options not present in 2020.06 are prefixed always.
    option_prefix = build.head? ? 'MAGNUM_' : ''
    # 2020.06 has CMake 3.5 as minimum required for backwards compatibility
    # purposes, but it works with any newer. CMake 4.0 removed compatibility
    # with it and suggests this as an override.
    # TODO remove once a new release is finally made
    extra_cmake_args = build.head? ? [] : ['-DCMAKE_POLICY_VERSION_MINIMUM=3.5']

    system "mkdir build"
    cd "build" do
      system "cmake",
        *(std_cmake_args + extra_cmake_args),
        # Without this, ARM builds will try to look for dependencies in
        # /usr/local/lib and /usr/lib (which are the default locations) instead
        # of /opt/homebrew/lib which is dedicated for ARM binaries. Please
        # complain to Homebrew about this insane non-obvious filesystem layout.
        "-DCMAKE_INSTALL_NAME_DIR:STRING=#{lib}",
        "-D#{option_prefix}WITH_PLAYER=ON",
        "-D#{option_prefix}WITH_UI=ON",
        "-D#{option_prefix}WITH_UI_GALLERY=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
