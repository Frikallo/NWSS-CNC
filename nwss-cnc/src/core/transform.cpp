#include "core/transform.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

namespace nwss {
namespace cnc {

bool Transform::getBounds(const std::vector<Path> &paths, double &minX,
                          double &minY, double &maxX, double &maxY) {
  if (paths.empty()) {
    return false;
  }

  // Initialize with extreme values
  minX = std::numeric_limits<double>::max();
  minY = std::numeric_limits<double>::max();
  maxX = std::numeric_limits<double>::lowest();
  maxY = std::numeric_limits<double>::lowest();

  // Iterate through all paths and points to find bounds
  for (const auto &path : paths) {
    const auto &points = path.getPoints();
    for (const auto &point : points) {
      minX = std::min(minX, point.x);
      minY = std::min(minY, point.y);
      maxX = std::max(maxX, point.x);
      maxY = std::max(maxY, point.y);
    }
  }

  return (minX <= maxX && minY <= maxY);
}

bool Transform::fitToMaterial(std::vector<Path> &paths, const CNConfig &config,
                              bool preserveAspectRatio, bool centerX,
                              bool centerY, bool flipY, TransformInfo *info) {
  // Get material dimensions from config
  double materialWidth = config.getMaterialWidth();
  double materialHeight = config.getMaterialHeight();

  // Create transform info and initialize
  TransformInfo localInfo;
  if (info == nullptr) {
    info = &localInfo;
  }

  info->success = false;
  info->wasScaled = false;
  info->wasCropped = false;
  info->message.clear();

  // Get bounds of the original paths
  double minX, minY, maxX, maxY;
  if (!getBounds(paths, minX, minY, maxX, maxY)) {
    info->message = "Error: Could not determine bounds of the paths.";
    return false;
  }

  // Store original bounds in the transform info
  info->origMinX = minX;
  info->origMinY = minY;
  info->origWidth = maxX - minX;
  info->origHeight = maxY - minY;

  // Check if the design exceeds material dimensions
  bool exceedsWidth = (info->origWidth > materialWidth);
  bool exceedsHeight = (info->origHeight > materialHeight);

  // Calculate scaling factors
  double scaleX = materialWidth / info->origWidth;
  double scaleY = materialHeight / info->origHeight;

  // If preserving aspect ratio, use the smaller scale factor
  double scale = 1.0;
  if (exceedsWidth || exceedsHeight) {
    info->wasScaled = true;

    if (preserveAspectRatio) {
      scale = std::min(scaleX, scaleY);
    } else {
      // Apply different scaling for X and Y
      for (auto &path : paths) {
        std::vector<Point2D> &points =
            const_cast<std::vector<Point2D> &>(path.getPoints());
        for (auto &point : points) {
          point.x = (point.x - minX) * scaleX;
          point.y = (point.y - minY) * scaleY;
        }
      }

      // Update info for non-uniform scaling
      info->scaleX = scaleX;
      info->scaleY = scaleY;
      info->offsetX = -minX * scaleX;
      info->offsetY = -minY * scaleY;
      info->newWidth = info->origWidth * scaleX;
      info->newHeight = info->origHeight * scaleY;
      info->newMinX = 0;
      info->newMinY = 0;

      // Add positioning offset if centering
      if (centerX) {
        double offsetX = (materialWidth - info->newWidth) / 2.0;
        for (auto &path : paths) {
          std::vector<Point2D> &points =
              const_cast<std::vector<Point2D> &>(path.getPoints());
          for (auto &point : points) {
            point.x += offsetX;
          }
        }
        info->offsetX += offsetX;
        info->newMinX = offsetX;
      }

      if (centerY) {
        double offsetY = (materialHeight - info->newHeight) / 2.0;
        for (auto &path : paths) {
          std::vector<Point2D> &points =
              const_cast<std::vector<Point2D> &>(path.getPoints());
          for (auto &point : points) {
            point.y += offsetY;
          }
        }
        info->offsetY += offsetY;
        info->newMinY = offsetY;
      }

      // Flip Y coordinates if needed (CNC often has Y=0 at the front)
      if (flipY) {
        for (auto &path : paths) {
          std::vector<Point2D> &points =
              const_cast<std::vector<Point2D> &>(path.getPoints());
          for (auto &point : points) {
            point.y = materialHeight - point.y;
          }
        }
        info->newMinY = materialHeight - info->newMinY - info->newHeight;
      }

      info->success = true;
      return true;
    }
  }

  // For uniform scaling or no scaling needed
  // First, translate to origin
  double offsetX = -minX;
  double offsetY = -minY;

  // Apply scale if needed
  if (info->wasScaled) {
    for (auto &path : paths) {
      std::vector<Point2D> &points =
          const_cast<std::vector<Point2D> &>(path.getPoints());
      for (auto &point : points) {
        point.x = (point.x + offsetX) * scale;
        point.y = (point.y + offsetY) * scale;
      }
    }
  } else {
    // Just translate to origin
    for (auto &path : paths) {
      std::vector<Point2D> &points =
          const_cast<std::vector<Point2D> &>(path.getPoints());
      for (auto &point : points) {
        point.x = point.x + offsetX;
        point.y = point.y + offsetY;
      }
    }
  }

  // Update scaled dimensions
  info->newWidth = info->origWidth * (info->wasScaled ? scale : 1.0);
  info->newHeight = info->origHeight * (info->wasScaled ? scale : 1.0);
  info->scaleX = info->scaleY = (info->wasScaled ? scale : 1.0);
  info->offsetX = offsetX;
  info->offsetY = offsetY;
  info->newMinX = 0;
  info->newMinY = 0;

  // Add positioning offset if centering
  if (centerX) {
    double centerOffsetX = (materialWidth - info->newWidth) / 2.0;
    for (auto &path : paths) {
      std::vector<Point2D> &points =
          const_cast<std::vector<Point2D> &>(path.getPoints());
      for (auto &point : points) {
        point.x += centerOffsetX;
      }
    }
    info->offsetX += centerOffsetX;
    info->newMinX = centerOffsetX;
  }

  if (centerY) {
    double centerOffsetY = (materialHeight - info->newHeight) / 2.0;
    for (auto &path : paths) {
      std::vector<Point2D> &points =
          const_cast<std::vector<Point2D> &>(path.getPoints());
      for (auto &point : points) {
        point.y += centerOffsetY;
      }
    }
    info->offsetY += centerOffsetY;
    info->newMinY = centerOffsetY;
  }

  // Flip Y coordinates if needed (CNC often has Y=0 at the front)
  if (flipY) {
    for (auto &path : paths) {
      std::vector<Point2D> &points =
          const_cast<std::vector<Point2D> &>(path.getPoints());
      for (auto &point : points) {
        point.y = materialHeight - point.y;
      }
    }
    info->newMinY = materialHeight - info->newMinY - info->newHeight;
  }

  // Generate message with transform information
  std::stringstream msgStream;
  if (info->wasScaled) {
    msgStream << "Design was scaled to fit material (scale factor: "
              << std::fixed << std::setprecision(4) << info->scaleX << ").";
  } else {
    msgStream << "Design fits within material dimensions without scaling.";
  }

  // Check if the design exceeds bed dimensions
  double bedWidth = config.getBedWidth();
  double bedHeight = config.getBedHeight();

  if (info->newWidth > bedWidth || info->newHeight > bedHeight) {
    msgStream << " WARNING: Design exceeds bed dimensions!";
    info->wasCropped = true;
  }

  info->message = msgStream.str();
  info->success = true;
  return true;
}

std::string Transform::formatTransformInfo(const TransformInfo &info,
                                           const CNConfig &config) {
  std::stringstream ss;
  std::string units = config.getUnitsString();

  ss << "Transform Information:" << std::endl;
  ss << "---------------------" << std::endl;

  // Original dimensions
  ss << "Original dimensions: " << std::fixed << std::setprecision(3)
     << info.origWidth << " x " << info.origHeight << " " << units << std::endl;

  ss << "Original position: (" << info.origMinX << ", " << info.origMinY << ") "
     << units << std::endl;

  // Transformed dimensions
  ss << "New dimensions: " << std::fixed << std::setprecision(3)
     << info.newWidth << " x " << info.newHeight << " " << units
     << (info.wasScaled ? " (scaled)" : "") << std::endl;

  ss << "New position: (" << info.newMinX << ", " << info.newMinY << ") "
     << units << std::endl;

  // Scale factors
  if (info.wasScaled) {
    ss << "Scale factors: "
       << "X: " << info.scaleX << ", Y: " << info.scaleY << std::endl;
  }

  // Material size
  ss << "Material size: " << config.getMaterialWidth() << " x "
     << config.getMaterialHeight() << " " << units << std::endl;

  // Bed size
  ss << "Bed size: " << config.getBedWidth() << " x " << config.getBedHeight()
     << " " << units << std::endl;

  // Warning if needed
  if (info.wasCropped) {
    ss << std::endl
       << "WARNING: The design exceeds the bed dimensions!" << std::endl;
    ss << "         Some parts of the design may be cut off." << std::endl;
  }

  return ss.str();
}

}  // namespace cnc
}  // namespace nwss